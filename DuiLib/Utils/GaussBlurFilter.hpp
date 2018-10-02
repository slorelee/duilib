#pragma once

#include <stdlib.h>

#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include <math.h>

class IImgFilter
{
protected:
    bool m_bMultiThreads;
    int m_nThreadCount;

public:
    IImgFilter()
    {
        m_nThreadCount = 1;
    }
    virtual ~IImgFilter() { };
    virtual void Init(LPVOID pInfo) { };
    virtual void Reset() { };

    int GetThreadCount() const { return m_nThreadCount; }

    void SetThreads(int ThreadCount)
    {
        m_nThreadCount = ThreadCount;
        m_bMultiThreads = (m_nThreadCount > 1);

    }


    virtual bool Filter(LPCVOID pSrc, LPVOID pDest, int width, int height, int bpp)
    {
        return false;
    }
};

//
// Thread Function Declares;
//

template<typename T>
DWORD WINAPI GaussBlurThreadProc8(LPVOID lpParameters);

template<typename T>
DWORD WINAPI GaussBlurThreadProc24(LPVOID lpParameters);

//
// 传递到线程的参数
//
template<typename T>
class CGaussBlurThreadParams
{
public:
    int r;
    T* pTempl;
    LPBYTE pSrc;
    LPBYTE pDest;
    int width;
    int height;
    int rowBegin;
    int rowEnd;
    int stride;
    int pixelSize;
    bool bHorz;
public:
    CGaussBlurThreadParams() { };
    ~CGaussBlurThreadParams() { };
};


template<typename T>
class CGaussBlurImgFilter : public IImgFilter
{
protected:
    int m_r;
    T m_sigma;
    T* m_pTempl;

public:
    CGaussBlurImgFilter();
    virtual ~CGaussBlurImgFilter();

    int GetR() const { return m_r; };
    T GetSigma() const { return m_sigma; };
    void SetSigma(T sigma);

    virtual void Init(LPVOID pInfo);
    virtual void Reset();
    virtual bool Filter(LPCVOID pSrc, LPVOID pDest, int width, int height, int bpp);
};

template<typename T>
CGaussBlurImgFilter<T>::CGaussBlurImgFilter()
{
    m_r = -1;
    m_sigma = (T)(-1);
    m_pTempl = NULL;
    m_bMultiThreads = false;
    m_nThreadCount = 0;
}

template<typename T>
CGaussBlurImgFilter<T>::~CGaussBlurImgFilter()
{
    if (m_pTempl != NULL)
        free(m_pTempl);
}

template<typename T>
void CGaussBlurImgFilter<T>::SetSigma(T sigma)
{
    int i;
    m_sigma = sigma;
    m_r = (int)(m_sigma * 3 + 0.5);
    if (m_r <= 0) m_r = 1;

    LPVOID pOldTempl = m_pTempl;
    m_pTempl = (T*)realloc(m_pTempl, sizeof(T) * (m_r + 1));

    if (m_pTempl == NULL) {
        if (pOldTempl != NULL)
            free(pOldTempl);

        return;
    }

    T k1 = (T)((-0.5) / (m_sigma * m_sigma));
    for (i = 0; i <= m_r; i++)
        m_pTempl[i] = exp(k1 * i * i);

    T sum = m_pTempl[0];
    for (i = 1; i <= m_r; i++) {
        sum += (m_pTempl[i] * 2);
    }

    sum = (T)(1.0 / sum);
    for (i = 0; i <= m_r; i++)
        m_pTempl[i] *= sum;
}


template<typename T>
void CGaussBlurImgFilter<T>::Init(LPVOID pInfo)
{
    T* pT = (T*)pInfo;
    SetSigma(*pT);
}

template<typename T>
void CGaussBlurImgFilter<T>::Reset()
{
    m_r = -1;
    m_sigma = (T)(-1.0);
    if (m_pTempl != NULL) {
        free(m_pTempl);
        m_pTempl = NULL;
    }
}

template<typename T>
bool CGaussBlurImgFilter<T>::Filter(LPCVOID pSrc, LPVOID pDest, int width, int height, int bpp)
{
    if (pSrc == NULL || pDest == NULL)
        return false;

    if (bpp != 24 && bpp != 8 && bpp != 32)
        return false;

    if (m_r < 0 || m_pTempl == NULL)
        return false;

    int absHeight = (height >= 0) ? height : (-height);
    int stride = (width * bpp + 31) / 32 * 4;
    int pixelSize = bpp / 8;
    int i, ThreadCount;
    DWORD dwTid;

    LPVOID pTemp = malloc(stride * absHeight);
    if (pTemp == NULL)
        return false;


    if (m_bMultiThreads && m_nThreadCount > 1) {
        ThreadCount = min(m_nThreadCount, absHeight);

        CGaussBlurThreadParams<T> *p1 = new CGaussBlurThreadParams<T>[ThreadCount];
        HANDLE *pHandles = new HANDLE[ThreadCount];

        for (i = 0; i < ThreadCount; i++) {
            p1[i].pSrc = (LPBYTE)pSrc;
            p1[i].pDest = (LPBYTE)pTemp;
            p1[i].width = width;
            p1[i].height = absHeight;
            p1[i].stride = stride;
            p1[i].pixelSize = pixelSize;
            p1[i].r = m_r;
            p1[i].pTempl = m_pTempl;


            p1[i].rowBegin = absHeight / ThreadCount * i;

            if (i == ThreadCount - 1)
                p1[i].rowEnd = absHeight;
            else
                p1[i].rowEnd = p1[i].rowBegin + absHeight / ThreadCount;

            p1[i].bHorz = true;

            //Committed StackSize = 512;
            pHandles[i] = CreateThread(NULL, 512,
                (bpp == 8) ? GaussBlurThreadProc8<T> : GaussBlurThreadProc24<T>,
                (LPVOID)(&p1[i]), 0, &dwTid);
        }
        WaitForMultipleObjects(ThreadCount, pHandles, TRUE, INFINITE);
        for (i = 0; i < ThreadCount; i++)
            CloseHandle(pHandles[i]);

        for (i = 0; i < ThreadCount; i++) {
            p1[i].pSrc = (LPBYTE)pTemp;
            p1[i].pDest = (LPBYTE)pDest;
            p1[i].bHorz = false;

            pHandles[i] = CreateThread(NULL, 512,
                (bpp == 8) ? GaussBlurThreadProc8<T> : GaussBlurThreadProc24<T>,
                (LPVOID)(&p1[i]), 0, &dwTid);
        }
        WaitForMultipleObjects(ThreadCount, pHandles, TRUE, INFINITE);
        for (i = 0; i < ThreadCount; i++)
            CloseHandle(pHandles[i]);

        delete[] p1;
        delete[] pHandles;
    } else {

        CGaussBlurThreadParams<T> params;

        params.pSrc = (LPBYTE)pSrc;
        params.pDest = (LPBYTE)pTemp;
        params.width = width;
        params.height = absHeight;
        params.stride = stride;
        params.pixelSize = pixelSize;
        params.r = m_r;
        params.pTempl = m_pTempl;
        params.rowBegin = 0;
        params.rowEnd = absHeight;
        params.bHorz = true;

        if (bpp == 8)
            GaussBlurThreadProc8<T>(&params);
        else
            GaussBlurThreadProc24<T>(&params);

        params.pSrc = (LPBYTE)pTemp;
        params.pDest = (LPBYTE)pDest;
        params.bHorz = false;

        if (bpp == 8)
            GaussBlurThreadProc8<T>(&params);
        else
            GaussBlurThreadProc24<T>(&params);
    }

    free(pTemp);
    return true;
}

//thread entry: 8 bpp
template<typename T>
DWORD WINAPI GaussBlurThreadProc8(LPVOID lpParameters)
{
    CGaussBlurThreadParams<T> *pInfo = (CGaussBlurThreadParams<T>*)lpParameters;

    T result;
    int row, col, subRow, subCol, MaxVal, x, x1;
    LPINT pSubVal, pRefVal;

    if (pInfo->bHorz) {
        pSubVal = &subCol;
        pRefVal = &col;
        MaxVal = pInfo->width - 1;
    } else {
        pSubVal = &subRow;
        pRefVal = &row;
        MaxVal = pInfo->height - 1;
    }

    LPBYTE pSrcPixel = NULL;
    LPBYTE pDestPixel = NULL;

    for (row = pInfo->rowBegin; row < pInfo->rowEnd; ++row) {
        for (col = 0; col < pInfo->width; ++col) {
            pDestPixel = pInfo->pDest + pInfo->stride * row + col;

            result = 0;

            subRow = row;
            subCol = col;

            for (x = -pInfo->r; x <= pInfo->r; x++) {
                x1 = (x >= 0) ? x : (-x);
                *pSubVal = *pRefVal + x;
                if (*pSubVal < 0) *pSubVal = 0;
                else if (*pSubVal > MaxVal) *pSubVal = MaxVal;

                pSrcPixel = pInfo->pSrc + pInfo->stride * subRow + subCol;

                result += *pSrcPixel * pInfo->pTempl[x1];
            }
            *pDestPixel = (BYTE)result;
        }
    }
    return 0;
}

//thread entry: 24 bpp
template<typename T>
DWORD WINAPI GaussBlurThreadProc24(LPVOID lpParameters)
{
    CGaussBlurThreadParams<T> *pInfo = (CGaussBlurThreadParams<T>*)lpParameters;

    T result[3];
    int row, col, subRow, subCol, MaxVal, x, x1;
    LPINT pSubVal, pRefVal;

    if (pInfo->bHorz) {
        pSubVal = &subCol;
        pRefVal = &col;
        MaxVal = pInfo->width - 1;
    } else {
        pSubVal = &subRow;
        pRefVal = &row;
        MaxVal = pInfo->height - 1;
    }

    LPBYTE pSrcPixel = NULL;
    LPBYTE pDestPixel = NULL;

    for (row = pInfo->rowBegin; row < pInfo->rowEnd; ++row) {
        for (col = 0; col < pInfo->width; ++col) {
            pDestPixel = pInfo->pDest + pInfo->stride * row + pInfo->pixelSize * col;

            result[0] = 0;
            result[1] = 0;
            result[2] = 0;

            subRow = row;
            subCol = col;

            for (x = -pInfo->r; x <= pInfo->r; x++) {
                x1 = (x >= 0) ? x : (-x);
                *pSubVal = *pRefVal + x;

                if (*pSubVal < 0) *pSubVal = 0;
                else if (*pSubVal > MaxVal) *pSubVal = MaxVal;

                pSrcPixel = pInfo->pSrc + pInfo->stride * subRow + pInfo->pixelSize * subCol;

                result[0] += pSrcPixel[0] * pInfo->pTempl[x1];
                result[1] += pSrcPixel[1] * pInfo->pTempl[x1];
                result[2] += pSrcPixel[2] * pInfo->pTempl[x1];
            }
            pDestPixel[0] = (BYTE)result[0];
            pDestPixel[1] = (BYTE)result[1];
            pDestPixel[2] = (BYTE)result[2];
        }
    }
    return 0;
}
