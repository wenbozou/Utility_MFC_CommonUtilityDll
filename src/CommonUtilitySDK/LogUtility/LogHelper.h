/**************************************************************
 *  Filename:    @OutputName.cpp
 *  Copyright:   @Company Co., Ltd.
 *
 *  Description: ���̰߳�ȫ��.
 *
 *  @author:     @WenboZou
 *  @version     @MM/@dd/@yyyy  @Reviser  Initial Version
 **************************************************************/

#pragma once
#include <string>
#include <fstream>
#include <mutex>

namespace CommonUtilitySDK {

	class CLogHelper
	{
	protected:
		CLogHelper();

	public:
		~CLogHelper();

		void WriteLog(const std::string &strLocal, const std::string &strEventMsg);
		void WriteLog(const std::wstring &wstrLocal, const std::wstring &wstrEventMsg);

		static CLogHelper* Instance();
		static void UnInstance();
	private:
		void InitLogsFile();
		void GetCurTime(std::string& strTime);

	private:
		static CLogHelper *m_pLogHelpInstance;	//��־����ʵ��

		std::ofstream m_of;	//����ļ���
		std::wstring m_strLogFile;	//��־�ļ�������·��

		/*
		class CGarbo
		{
		public:
			~CGarbo()
			{
				if (CLogHelper::m_pLogHelpInstance)
				{
					delete CLogHelper::m_pLogHelpInstance;
					CLogHelper::m_pLogHelpInstance = NULL;
				}
			}
		};

		static CGarbo m_sGarbo;
		//*/
	};

}