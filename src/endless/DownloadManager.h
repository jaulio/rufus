#pragma once

#include <windows.h>
#include <bits.h>

/// IUnknown methods declaration macro
#define DECLARE_IUNKNOWN \
    STDMETHOD_(ULONG, AddRef)(void); \
    STDMETHOD_(ULONG, Release)(void); \
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID *ppvObj);

typedef enum DownloadType {
    DownloadTypeReleseJson,
    DownloadTypeLiveImage,
    DownloadTypeInstallerImage,
    DownloadTypeMax
} DownloadType_t;

// helper methods
#define TOSTR(value) case value: return _T(#value)

static LPCTSTR DownloadTypeToString(DownloadType_t type)
{
    switch (type)
    {
        TOSTR(DownloadTypeReleseJson);
        TOSTR(DownloadTypeLiveImage);
        TOSTR(DownloadTypeInstallerImage);
        default: return _T("UNKNOWN_DOWNLOAD_TYPE");
    }
}

typedef struct DownloadStatus {
    BG_JOB_PROGRESS progress;    
    bool done;
    bool error;
    CString jobName;
} DownloadStatus_t;

static const DownloadStatus_t DownloadStatusNull = { {0, 0, 0, 0}, false, false, L"" };

class DownloadManager : public IBackgroundCopyCallback {
public:

    DownloadManager();
    ~DownloadManager();

    bool Init(HWND window, DWORD statusMessageId);
    bool Uninit();

    bool AddDownload(DownloadType_t type, LPCTSTR url, LPCTSTR file, bool startDownload = false, bool *appendFile = NULL, LPCTSTR jobSuffix = NULL);

    static CString GetJobName(DownloadType_t type);

    // IUnknown implementation
    DECLARE_IUNKNOWN;

    // IBackgroundCopyCallback implementation
    STDMETHOD(JobTransferred)(IBackgroundCopyJob *pJob);
    STDMETHOD(JobError)(IBackgroundCopyJob *pJob, IBackgroundCopyError *pError);
    STDMETHOD(JobModification)(IBackgroundCopyJob *JobModification, DWORD dwReserved);

private:
    LONG m_PendingJobModificationCount;
    static volatile ULONG m_refCount;
    HWND m_dispatchWindow;
    DWORD m_statusMsgId;
    CComPtr<IBackgroundCopyManager> m_bcManager;
    CComPtr<IBackgroundCopyJob> m_bcReleaseJson;

    HRESULT GetExistingJob(LPCWSTR jobName, CComPtr<IBackgroundCopyJob> &existingJob);
    void ClearExtraDownloadJobs();
};