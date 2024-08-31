#pragma once
#include <string>
#include <vector>


struct JobInfo;

class CCurlDownloader
{
public:
	CCurlDownloader();
	virtual ~CCurlDownloader();

	void DownloadStart(const char* urlUtf8, const char* localFileUtf8);
	void Stop();
protected:
	static void set_JobInfo(JobInfo* ji, FILE* f, long long s, long long e, std::string u, bool r);
	static long long get_download_file_length(const char *url);
	static long long writeCallback(void* contents, size_t size, size_t nmemb, void* userp);
	static size_t write_data(void* ptr, size_t size, size_t nmemb, void* userdata);
	static void* job_process(void* arg);
	int get_job_queue(std::vector<std::string>& urls, std::vector<JobInfo*>& jobs, const char* localFileUtf8);
	static int downloadProgress(void *p, long long dltotal, long long dlnow, long long ultotal, long long ulnow);
	//static void downloadTask(const std::string& url, const std::string& filename, void* p);
	
protected:
	static bool m_bStop;
	static bool m_bCanWriteDataLog;
};