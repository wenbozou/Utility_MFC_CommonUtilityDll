#include "pch.h"
#include "ThreadPool/threadpoolT.h"
#include "CurlDownloader.h"
#include "curl.h"
#include <iostream>
#include <fstream>
#include <thread>
#include <mutex>
#include <map>
//#include <functional>
#include "StringConversion.h"

using namespace std;
#pragma warning (disable: 4996)

bool CCurlDownloader::m_bStop = false;
bool CCurlDownloader::m_bCanWriteDataLog = false;
// ȫ�ֱ���
std::recursive_mutex mtx;  // �����������ڱ���������Դ
std::ofstream file;  // �ļ�������

#define  HTTP_RANGE    1 //�Ƿ���http range���ع���?
const unsigned long seg_size = 20 * 1024 * 1024; //�ļ���Ƭ��ֵ��С��100MB, �����ô�С���ļ�����ǿ�Ʒ�Ƭ����

std::recursive_mutex g_mutex;
map<string, int>  download_map; //���������������, key��url�����ļ���, val�Ƿ�Ƭ��, �������̼߳���

class CloseInfo {
public:
	CloseInfo(FILE* f, string& s) : fp(f), url(s) {}
	~CloseInfo() {}
public:
	FILE*    fp;
	string   url;
};
vector<CloseInfo*> closed_vec; //���д򿪵��ļ��б���Ҫ�����йر�, ��Ϊ�ڷ�Ƭ���ص������,�޷���֤�����еķ�Ƭ��������ɵ�����¹ر��ļ�

//���������struct��������class
struct JobInfo {
	JobInfo() : fp(NULL), startPos(0), stopPos(0), ranged(false) {}

	FILE*    fp;  //�����ļ����
	unsigned long long     startPos;
	unsigned long long     stopPos;
	string   url;
	bool     ranged; //�Ƿ���Ҫhttp range����?
};


CCurlDownloader::CCurlDownloader()
{
	m_bStop = false;
	curl_global_init(CURL_GLOBAL_ALL);
}

CCurlDownloader::~CCurlDownloader()
{
	curl_global_cleanup();
}

void CCurlDownloader::DownloadStart(const char* urlUtf8, const char* localFileUtf8)
{
	m_bStop = false;
	//curl_global_init(CURL_GLOBAL_ALL);
	vector<string> urls_vec;
	urls_vec.push_back(urlUtf8);
	//�����߳�Խ��, cpuռ����Խ��, ����Ч�ʻ���һ�����
	//CThreadPool* pool = new CThreadPool(30);
	vector<JobInfo*> ji_vec;
	get_job_queue(urls_vec, ji_vec, localFileUtf8);
	vector<JobInfo*>::iterator it;
	threadpool pool(1);
	std::vector<std::future<JobInfo*>> results;
	int i = 0;
	for (it = ji_vec.begin(); it != ji_vec.end(); ++it, ++i) {
		JobInfo* ji = *it;
		//if (i == 1)
		{
			//pool->pool_add_job(job_process, (void*)ji);
			results.emplace_back(
				pool.commit([ji] {
				job_process(ji);
				return ji;
			})
			);
		}
		
	}
	//���ʱ�����һ������ֵ,�����߳������ļ�δ���ʱ,���worker�߳��ǲ����˳���, master�̻߳�һֱ�ȴ�, ֱ�����е�worker�̶߳��˳�
	//�˴����õ�˼·��ͳ�����е������Ƿ��������, ��׼���˳�
	//usleep(2000 * 1000 * 1000); //2000
	map<string, int>::iterator mit;
	int finished = 0;
	size_t map_size = download_map.size();
	printf("map size %ld\n", map_size);
	while (1) {
		finished = 0;
		//pthread_mutex_lock(&g_mutex);
		{
			std::lock_guard<std::recursive_mutex> lock(g_mutex);
			for (mit = download_map.begin(); mit != download_map.end(); ++mit)
				if (!mit->second)  finished++;
		}
		//pthread_mutex_unlock(&g_mutex);

		if (finished == map_size) {
			printf("finish all task =====================>\n");
			break;
		}
		else {
			Sleep(30 * 1000 * 1000);
		}
	}
	download_map.clear();

	//delete pool;
	urls_vec.clear();
	//ע��������jobinfo�Ѿ����̴߳�������ɾ����,���ﲻ���ٵ���ɾ��.JobInfo����������ʱ��,��رմ򿪵��ļ�.
	//�����̳߳�����ʱ,�Ὣʣ��û�д��������ɾ��
	ji_vec.clear();

	//�򿪵��ļ�,Ҫ�ǵ�ȫ���ر�
	vector<CloseInfo*>::iterator cit;
	for (cit = closed_vec.begin(); cit != closed_vec.end(); ++cit) {
		CloseInfo* ci = *cit;
		if (ci->fp != NULL) {
			fclose(ci->fp);
			ci->fp = NULL;
		}
		printf("[closed] %s\n", ci->url.c_str());
		delete ci;
	}
	closed_vec.clear();
	//curl_global_cleanup();
	/*
	const int numThreads = 4;   // �߳���Ŀ
	std::vector<std::thread> threads;
	
	// ��������߳�ִ����������
	for (int i = 0; i < numThreads; ++i) {
		threads.emplace_back(downloadTask, urlUtf8, localFileUtf8, this);
	}
	// �ȴ������߳����
	for (auto& thread : threads) {
		thread.join();
	}
	//*/
}

void CCurlDownloader::Stop()
{
	m_bStop = true;
}

void CCurlDownloader::set_JobInfo(JobInfo* ji, FILE* f, long long s, long long e, string u, bool r) {
	ji->fp = f;
	ji->startPos = s;
	ji->stopPos = e;
	ji->url = u;
	ji->ranged = r;
}

long long CCurlDownloader::get_download_file_length(const char *url)
{
	double file_len = 0;
	CURL *handle = curl_easy_init();
	curl_easy_setopt(handle, CURLOPT_URL, url);
	curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(handle, CURLOPT_MAXREDIRS, 3L);
	curl_easy_setopt(handle, CURLOPT_AUTOREFERER, 1L);
	curl_easy_setopt(handle, CURLOPT_HEADER, 0L);   //ֻ��Ҫheaderͷ
	curl_easy_setopt(handle, CURLOPT_NOBODY, 1L);   //����Ҫbody
	curl_easy_setopt(handle, CURLOPT_FORBID_REUSE, 1);
	//curl_easy_setopt(handle, CURLOPT_USERAGENT, "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/36.0.1985.143 Safari/537.36"); //user-agent
	if (curl_easy_perform(handle) == CURLE_OK)
	{
		curl_easy_getinfo(handle, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &file_len);
	}
	else
	{
		file_len = -1;
	}
	curl_easy_cleanup(handle);
	return (long long)file_len;
}

long long CCurlDownloader::writeCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
	std::lock_guard<std::recursive_mutex> lock(mtx);  // ����
	//mtx.lock();
	file.write(static_cast<const char*>(contents), size * nmemb);  // д���ļ�
	//mtx.unlock();
	return size * nmemb;
}

//ÿ���߳�������ÿ���ļ���Ƭʱ,����ص��ú���
size_t CCurlDownloader::write_data(void* ptr, size_t size, size_t nmemb, void* userdata) {
	JobInfo* ji = (JobInfo*)userdata;
	bool ranged = ji->ranged;
	size_t written;
	//Ҫ��Ƭ���صĴ��ļ�, ��Ҫ����http range��
	if (ranged) {
		//���߳�дͬһ���ļ�, ��Ҫ����
		//pthread_mutex_lock(&g_mutex);
		std::lock_guard<std::recursive_mutex> lock(g_mutex);
		
		if (ji->startPos + size * nmemb <= ji->stopPos) {
			fseek(ji->fp, ji->startPos, SEEK_SET);
			written = fwrite(ptr, size, nmemb, ji->fp);
			fflush(ji->fp);
			ji->startPos += written;
			//TRACE(L"written:[%I32d] \n", written);
		}
		else {
			TRACE(L"statrPos:[%llu] stopPos:[%llu] nmemb:[%lu] size[%lu]\n", ji->startPos, ji->stopPos, nmemb, size);
			fseek(ji->fp, ji->startPos, SEEK_SET);
			written = fwrite(ptr, 1, ji->stopPos - ji->startPos + 1, ji->fp);
			fflush(ji->fp);
			ji->startPos += written;
			TRACE(L"written:[%lu] \n", written);
		}
		//pthread_mutex_unlock(&g_mutex);
	}
	else {
		if (m_bCanWriteDataLog)
		{
			TRACE(L"[%llu] [%llu] write_data Enter\n", ji->startPos, ji->stopPos);
		}
		fseek(ji->fp, ji->startPos, SEEK_SET);
		written = fwrite(ptr, size, nmemb, ji->fp);
		fflush(ji->fp);
		ji->startPos += written;
		if (m_bCanWriteDataLog)
		{
			m_bCanWriteDataLog = false;
			TRACE(L"[%llu] [%llu] write_data Leave\n", ji->startPos, ji->stopPos);
		}
	}
	return written;
}

void* CCurlDownloader::job_process(void* arg) {
	CURL* curl;
	CURLcode res;
	JobInfo* ji = (JobInfo*)arg;

	char range[64] = { 0 };
	if (ji->ranged) {
		snprintf(range, sizeof(range), "%llu-%llu", ji->startPos, ji->stopPos);
		TRACE(L"%llu-%llu\n", ji->startPos, ji->stopPos);
	}


	
	m_bCanWriteDataLog = true;

	curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_URL, ji->url.c_str());
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)ji);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 3L);
	curl_easy_setopt(curl, CURLOPT_AUTOREFERER, 1L);
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
	curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 1L);
	curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, 5L);
	//curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/36.0.1985.143 Safari/537.36"); //user-agent
	//curl_easy_setopt(curl, CURLOPT_FORBID_REUSE, 1L);
	curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, downloadProgress);
	/* pass the struct pointer into the xferinfo function */
	curl_easy_setopt(curl, CURLOPT_XFERINFODATA, (void*)ji);
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
	//curl_easy_setopt(curl, CURLOPT_RESUME_FROM, ji->startPos);
	//curl_easy_setopt(curl, CURLOPT_RESUME_FROM_LARGE, local_file_len);

	if (ji->ranged)
		curl_easy_setopt(curl, CURLOPT_RANGE, range);

	res = curl_easy_perform(curl);
	if (CURLE_OK != res) {
		printf("[%d] %s\n", res, ji->url.c_str());
	}
	TRACE(L"[%d] %s\n", res, L"curl_easy_perform return");
	curl_easy_cleanup(curl);

	//TODO: �������ط�Ƭ��������....
	string url = ji->url;
	//pthread_mutex_lock(&g_mutex);
	std::lock_guard<std::recursive_mutex> lock(g_mutex);
	download_map[url]--;
	printf("[-1]%s\n", url.c_str());
	//pthread_mutex_unlock(&g_mutex);

	return NULL;
}

//��url�б�õ�һ����Ӧ�����������б�
int CCurlDownloader::get_job_queue(vector<string>& urls, vector<JobInfo*>& jobs, const char* localFileUtf8) {
	unsigned __int64 local_file_len = 0;
	std::string filename = localFileUtf8;  // ���غ󱣴���ļ���
	ifstream ifs;
	ifs.open(filename);
	if (ifs.is_open())
	{
		ifs.seekg(0, ios::end);
		local_file_len = ifs.tellg();
		ifs.seekg(0, ios::beg);
		ifs.close();
	}
	
	FILE* fp;
	unsigned __int64 file_len = 0, start, stop, seg_num = 0;
	bool ranged = false;
	JobInfo* ji;
	vector<string>::iterator it;
	for (it = urls.begin(); it != urls.end(); ++it) {
		string url = *it;
		file_len = get_download_file_length(url.c_str());
		if (file_len <= 0) continue;
		//printf("[%ld] %s\n", file_len, url.c_str());

		//��Ӧÿ��url, ��һ�������ļ�
		//const char* fn = "";//basename((char*)url.c_str());
		string full_path(localFileUtf8);
		//full_path = "./" + full_path; //��Ϊ��ǰ·������
		fp = fopen(full_path.c_str(), "ab"); //�ڴ�ͳһ���ļ�
		if (NULL == fp)  continue;

		//����ر��ļ�����
		CloseInfo* ci = new CloseInfo(fp, url);
		closed_vec.push_back(ci);

		//����ȫ���������ӳ��
		int additional = (file_len % seg_size == 0) ? 0 : 1;
		int seg_total = ranged ? (file_len < seg_size ? 1 : (file_len / seg_size + additional)) : 1;
		download_map[url] = seg_total;
		//printf("[+1]%s, seg total %d\n", url.c_str(), seg_total);

#if HTTP_RANGE
		//�����ļ���С, ȷ���Ƿ��Ƭ?
		if (file_len < seg_size) {
			start = 0;
			stop = file_len - 1;
			ji = new JobInfo();
			set_JobInfo(ji, fp, start, stop, url, ranged);
			jobs.push_back(ji);
		}
		else {
			//��Ƭ����,��ȷ����Ƭ����
			ranged = true;
			seg_num = file_len / seg_size;
			//TRACE(L"filesize[%lld], segsize[%lld], seg_num:[I32]\n", file_len, seg_size, seg_num);
			for (unsigned long long i = 0; i <= seg_num; i++) {
				if (i < seg_num) {
					start = i * seg_size;
					stop = (i + 1) * seg_size - 1;
				}
				else {
					if (file_len % seg_size != 0) {
						start = i * seg_size;
						stop = file_len - 1;
					}
					else
						break;
				}
				TRACE(L"[%llu]-[%llu] [%d]\n", start, stop, i);
				ji = new JobInfo();
				set_JobInfo(ji, fp, start, stop, url, ranged);
				jobs.push_back(ji);
			}
		}
#else
		start = local_file_len;
		stop = file_len - 1;
		ji = new JobInfo();
		set_JobInfo(ji, fp, start, stop, url, ranged);
		jobs.push_back(ji);
#endif
	}

	return 0;
}

int CCurlDownloader::downloadProgress(void *p, long long dltotal, long long dlnow, long long ultotal, long long ulnow)
{
	if (m_bStop) return -1;

	JobInfo *pJI = (JobInfo*)p;

	CString strProgress;
	//strProgress.Format(L"url=%s, startPos=%lld, stopPos=%lld, dltotal=%lld, dlnow=%lld", CommonUtilitySDK::CStringConversion::AnsiToUnicode(pJI->url.c_str()), pJI->startPos, pJI->stopPos, dltotal, dlnow);
	//if(dltotal > 0) strProgress.Format(L"startPos=%lld, stopPos=%lld, dltotal=%lld, dlnow=%lld, rate1=%f, rate2=%f\n",  pJI->startPos, pJI->stopPos, dltotal, dlnow, 100.0*pJI->startPos/pJI->stopPos, 100.0*dlnow/dltotal);
	//CommonUtilitySDK::CStringConversion::AnsiToUnicode();
	//TRACE(strProgress);
	return 0;
}
/*
void CCurlDownloader::downloadTask(const std::string& url, const std::string& filename, void* p)
{
	CURL* curl = curl_easy_init();
	if (curl) {
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, p);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
		std::lock_guard<std::recursive_mutex> lock(mtx);  // ����
		file.open(filename, std::ios::binary | std::ios_base::app);  // ���ļ�
		//if (file.is_open())
		{
			CURLcode res = curl_easy_perform(curl);
			file.close();  // �ر��ļ�
			if (res != CURLE_OK)
				fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
		}
		
		curl_easy_cleanup(curl);
	}
}//*/
