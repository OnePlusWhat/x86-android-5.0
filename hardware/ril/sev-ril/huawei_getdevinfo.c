


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <fcntl.h>

#define LOG_TAG "RIL"
#include <utils/Log.h>

typedef enum {
	PID_OLD,//��PID ����
	PID_NEW,//��PID ����
	PID_UNKNOWN
}PID_Project;

/***************************************************
 Function:  getportonpid
 Description:  
    Obtain pcui port by usb device pid 
 Calls: 
 Called By:
    RIL_Init
 Input:
    none
 Output:
    none
 Return:
    none
 Others:
    add by lizheng lkf33986 2010-11-13
    modify by fKF34305 20111111
    modify by lkf52201 2011-12-31
**************************************************/
int huawei_getportonpid()
{
	struct MODEMP
	{
		char number[10];
		char index[10];
	};
	struct DIAGP
	{
	 	char number[10];
	 	char index[10];
	};
	struct PCUIP
	{
		char number[10];
		char index[10];
	};
	struct DEVICEPORTS
	{
		char PID[10];
		struct MODEMP modem;
		struct DIAGP diag;
		struct PCUIP pcui;
	};
	struct DEVICEPORTS Devices[]={
		#include "PID_table.h"
	};
    int ret = 0;
	DIR    *pDir1; // /sys/bus/usb/drivers/optionĿ¼�µ�Ŀ¼��
	//DIR    *pDir2; // /sys/bus/usb/drivers/optionĿ¼�µ�Ŀ¼��
	DIR    *pDir_acm;// /sys/bus/usb/drivers/option/x-x:x.x Ŀ¼�µ�Ŀ¼��
	DIR    *pDir_usb;// /sys/bus/usb/drivers/option/x-x:x.x/tty Ŀ¼�µ�Ŀ¼��
	char dirdir[1024] = ""; //��Ŀ¼����·��

	FILE   *pfmodalias;// ��/sys/bus/usb/drivers/option/%s/modalias �ļ�ָ��
	FILE   *pfbInterfaceNumber;// �� /sys/bus/usb/drivers/option/%s/bInterfaceNu ���ļ�ָ��
	FILE   *pfbInterfaceProtocol;// �� /sys/bus/usb/drivers/option/%s/bInterfaceProtocol ���ļ�ָ��
	

	char cdirmodalias[1024] = ""; //���ļ���·��
	char cdirbInterfaceNumber[1024] = "" ;
	char cdirbInterfaceProtocol[1024] = "" ;

	char cmodalias[1024] = ""; //����modalias�е�����
	char cbInterfaceNumber[1024] = "";//����bInterfaceNumber�е�����
	char cbInterfaceProtocol[1024] = "";//����bInterfaceNumber�е�����

	char dirtty[1024] = ""; //��Ŀ¼����·��
    int i;
	int MAX_PID = sizeof(Devices)/sizeof(Devices[0]);
	//LOGD("max_pid=%d",MAX_PID);
	struct dirent *pNext1 = NULL;
	//struct dirent *pNext2 = NULL;
	struct dirent *pNext_acm = NULL;
	struct dirent *pNext_usb = NULL;

	PID_Project p_project_status = PID_UNKNOWN;

	//�����ĳ�ʼ��
	memset(dirdir,'\0', sizeof(dirdir));
	memset(cdirmodalias,'\0', sizeof(cdirmodalias));
	memset(cdirbInterfaceNumber,'\0', sizeof(cdirbInterfaceNumber));
	memset(cdirbInterfaceProtocol,'\0', sizeof(cdirbInterfaceProtocol));
	memset(cmodalias,'\0', sizeof(cmodalias));
	memset(cbInterfaceNumber,'\0', sizeof(cbInterfaceNumber));
	memset(cbInterfaceProtocol,'\0', sizeof(cbInterfaceProtocol));
	memset(dirtty,'\0', sizeof(dirtty));
/*BEGIN DTS2013091610549 w84014221 2013-09-21 modified*/
	pDir_usb= opendir(DIR_SYS_BUS_USB_DRIVERS_OPTION);
	if (NULL == pDir_usb)
	{
		LOGD("opendir DIR_SYS_BUS_USB_DRIVERS_OPTION  fail");
	}
/*END DTS2013091610549 w84014221 2013-09-21 modified*/
	//�����ж��豸�Ƿ�ΪUSB����
	while(NULL != (pNext_usb= readdir(pDir_usb)))
	{
	    /*BEGIN DTS2013091610549 w84014221 2013-09-21 modified*/
		snprintf(dirdir,sizeof(dirdir) - 1, "/sys/bus/usb/drivers/option/%s", pNext_usb->d_name);  // ��ȡ/sys/bus/usb/drivers/option/x-x:x.x Ŀ¼��
		snprintf(cdirmodalias,sizeof(cdirmodalias) - 1, "/sys/bus/usb/drivers/option/%s/modalias", pNext_usb->d_name);  //�ļ�modalias·��
		snprintf(cdirbInterfaceProtocol,sizeof(cdirbInterfaceProtocol) - 1, "/sys/bus/usb/drivers/option/%s/bInterfaceProtocol", pNext_usb->d_name);  //�ļ�modalias·��
		snprintf(cdirbInterfaceNumber,sizeof(cdirbInterfaceNumber) - 1, "/sys/bus/usb/drivers/option/%s/bInterfaceNumber", pNext_usb->d_name);  //�ļ�modalias·��
		
		if(NULL == strstr(pNext_usb->d_name,":"))
		{
			continue;
		}
		/*END DTS2013091610549 w84014221 2013-09-21 modified*/
		/* Begin modified by x84000798 for PID solution for DTS2012112004053 2012-11-21*/
		//��ȥ��pfbInterfaceProtocol ��ֵ�ж�ģ����õ�PID������������PID���������ò�PID��
		if(NULL != (pfbInterfaceProtocol = fopen(cdirbInterfaceProtocol,"r")))
		{
			fgets(cbInterfaceProtocol,1024,pfbInterfaceProtocol);
			if(NULL != strstr(cbInterfaceProtocol,"ff"))      
			{
				p_project_status = PID_OLD;
			}
			else
			{
				p_project_status = PID_NEW;
			}
			fclose(pfbInterfaceProtocol); // DTS2011123100128 lkf52201 2011-12-31 added
		}
		//New PID Solution
		if (PID_NEW == p_project_status)
		{
			pDir1 = opendir(dirdir);
			while(NULL != (pNext1 = readdir(pDir1)))
			{								
				if(NULL != strstr(pNext1->d_name,"ttyUSB"))
				{
				   /*BEGIN DTS2013091610549 w84014221 2013-09-21 modified*/
					if(strstr(cbInterfaceProtocol,"01") || strstr(cbInterfaceProtocol,"10")) //modified by c00221986 for DTS2013061402496
					{
						snprintf(SERIAL_PORT_MODEM_KEYNAME, sizeof(SERIAL_PORT_MODEM_KEYNAME)-1, "/dev/%s", pNext1->d_name);  //coverity=SECURE_CODING
						LOGD("modem port is %s\n",SERIAL_PORT_MODEM_KEYNAME);
						ret |= 1 << 0 ;
					}
					else if(strstr(cbInterfaceProtocol,"02") || strstr(cbInterfaceProtocol,"12")) //modified by c00221986 for DTS2013061402496
					{
					    /*modified by huwen for fortify 2013-9-14, begin*/
						snprintf(SERIAL_PORT_PCUI_KEYNAME, sizeof(SERIAL_PORT_PCUI_KEYNAME)-1, "/dev/%s", pNext1->d_name);  //coverity=SECURE_CODING
						/*modified by huwen for fortify 2013-9-14, end*/
						LOGD("pcui port is %s\n",SERIAL_PORT_PCUI_KEYNAME);
						ret |= 1 << 1 ;//modify by KF34305 20111206 for DTS2011120604273
					}
					else if(strstr(cbInterfaceProtocol,"03") || strstr(cbInterfaceProtocol,"13")) //modified by c00221986 for DTS2013061402496
					{
					    /*modified by huwen for fortify 2013-9-14, begin*/
						snprintf(SERIAL_PORT_DIAG_KEYNAME, sizeof(SERIAL_PORT_DIAG_KEYNAME)-1, "/dev/%s", pNext1->d_name);  //coverity=SECURE_CODING
						/*modified by huwen for fortify 2013-9-14, end*/
						LOGD("diag port is %s\n",SERIAL_PORT_DIAG_KEYNAME);
						ret |= 1 << 2 ;//modify by KF34305 20111206 for DTS2011120604273
					}
					/*END DTS2013091610549 w84014221 2013-09-21 modified*/
					else
					{
						LOGD("----The port is not recognized----");
					}
				}
			}
			if (NULL != pDir1)
			{
			    closedir(pDir1);
			}			
		}
		/*
			�ɶ˿ڷ�����ACM�˿ڷ����ϲ�����sys/bus/usb/device·���²��Ҷ˿��豸
		*/
		//Old PID Solution
		/*
		else
		{
			if(NULL != (pfmodalias = fopen(cdirmodalias,"r")))
			{
				fgets(cmodalias,1024,pfmodalias);
				for(i = 0; i < MAX_PID; i++)
				{
					if(NULL != strstr(cmodalias,Devices[i].PID)) //��ѯPID
					{
						if(NULL != (pfbInterfaceNumber = fopen(cdirbInterfaceNumber,"r")))
						{
							fgets(cbInterfaceNumber,1024,pfbInterfaceNumber);
							pDir1 = opendir(dirdir);
							while(NULL != (pNext1 = readdir(pDir1)))
							{								
								if(NULL != strstr(pNext1->d_name,"ttyUSB"))
								{
									if(PID_OLD == p_project_status)     
									{
										//BEGIN DTS2013091610549 w84014221 2013-09-21 modified
										if(NULL != strstr(cbInterfaceNumber,Devices[i].modem.number)) 
										{
											snprintf(SERIAL_PORT_MODEM_KEYNAME, sizeof(SERIAL_PORT_MODEM_KEYNAME)-1, "/dev/%s", pNext1->d_name);  //coverity=SECURE_CODING
											LOGD("modem port is %s\n",SERIAL_PORT_MODEM_KEYNAME);
											ret |= 1 << 0 ;//modify by kf33986 DTS2010102503678 �����USB����������tyyUSB�豸û����ȫ�����Ͳ��Ҷ˿ڣ����¶˿��޷���
										}
										else if(NULL != strstr(cbInterfaceNumber,Devices[i].pcui.number))
										{
										    //modified by huwen for fortify 2013-9-14, begin
											snprintf(SERIAL_PORT_PCUI_KEYNAME, sizeof(SERIAL_PORT_MODEM_KEYNAME)-1, "/dev/%s",pNext1->d_name);
											//modified by huwen for fortify 2013-9-14, end
											LOGD("pcui port is %s\n",SERIAL_PORT_PCUI_KEYNAME);
											ret |= 1 << 1 ;//modify by kf33986 DTS2010102503678 �����USB����������tyyUSB�豸û����ȫ�����Ͳ��Ҷ˿ڣ����¶˿��޷���
										} 
										else if(NULL != strstr(cbInterfaceNumber,Devices[i].diag.number))
										{
										    //modified by huwen for fortify 2013-9-14, begin
											snprintf(SERIAL_PORT_DIAG_KEYNAME, sizeof(SERIAL_PORT_MODEM_KEYNAME)-1, "/dev/%s",pNext1->d_name);
											//modified by huwen for fortify 2013-9-14, end
											LOGD("diag port is %s\n",SERIAL_PORT_DIAG_KEYNAME);
											ret |= 1 << 2 ;//modify by kf33986 DTS2010102503678 �����USB����������tyyUSB�豸û����ȫ�����Ͳ��Ҷ˿ڣ����¶˿��޷���
										}
										//END DTS2013091610549 w84014221 2013-09-21 modified
									}									
								}
							}
				// BEGIN DTS2011123100128  lkf52201 2011-12-31 added 
							if (NULL != pDir1)
							{
							    closedir(pDir1);
							}
							fclose(pfbInterfaceNumber); 
						}
					}				
				}
				fclose(pfmodalias); 
				//END DTS2011123100128  lkf52201 2011-12-31 added 
			}
		}*/
		/* End modified by x84000798 for PID solution for DTS2012112004053 2012-11-21*/
	}
    /* BEGIN DTS2011123100128  lkf52201 2011-12-31 modified */
	if (NULL != pDir_usb)
	{
	    closedir(pDir_usb);
	}
	/* END DTS2011123100128  lkf52201 2011-12-31 modified */
	if(ret)
	{
		return ret;
	}
/*BEGIN DTS2013091610549 w84014221 2013-09-21 modified*/		
	//����ж��豸�Ƿ�ΪACM����
	pDir_acm = opendir(DIR_SYS_BUS_USB_DEVICES);
	
	if (NULL == pDir_acm)
	{
		LOGD("opendir DIR_SYS_BUS_USB_DEVICES fail");
	}
	
/*END DTS2013091610549 w84014221 2013-09-21 modified*/
/*BEGIN DTS2013083105452 w84014221 2013-10-26 for modified*/
	while(NULL != (pNext_acm= readdir(pDir_acm)))
	{
	    /*modified by huwen for DTS2013091610549 2013-9-14, begin*/
		snprintf(dirdir, sizeof(dirdir) -1, "/sys/bus/usb/devices/%s",pNext_acm->d_name);  // ��ȡ/sys/bus/usb/drivers/option/x-x:x.x Ŀ¼��
		snprintf(cdirmodalias, sizeof(cdirmodalias) -1, "/sys/bus/usb/devices/%s/modalias", pNext_acm->d_name);  //�ļ�modalias·��
		snprintf(cdirbInterfaceNumber, sizeof(cdirbInterfaceNumber) -1, "/sys/bus/usb/devices/%s/bInterfaceNumber", pNext_acm->d_name); //�ļ�bInterfaceNumber·��
        //snprintf(dirtty, sizeof(dirtty) -1, "/sys/bus/usb/drivers/cdc_acm/%s/tty",pNext_acm->d_name);
		/*modified by huwen for DTS2013091610549 2013-9-14, end*/

		if(NULL == strstr(pNext_acm->d_name,":"))
			continue;
		if(NULL != (pfmodalias = fopen(cdirmodalias,"r")))
		{
			fgets(cmodalias,1024,pfmodalias);
			for(i = 0; i < MAX_PID; i++)
			{
				if(NULL != strstr(cmodalias,Devices[i].PID)) //��ѯPID
				{
					if(NULL != (pfbInterfaceNumber = fopen(cdirbInterfaceNumber,"r")))
					{
						fgets(cbInterfaceNumber,1024,pfbInterfaceNumber);
						pDir1 = opendir(dirdir);

						while(NULL != (pNext1 = readdir(pDir1)))
						{   
							if( (NULL != strstr(pNext1->d_name,"ttyUSB")) || (NULL != strstr(pNext1->d_name,"ttyACM")) )
							{   							
							   // pDir2 = opendir(dirtty);
								//while(NULL != (pNext2 = readdir(pDir2)))
								//{										
									//if(NULL != strstr(pNext2->d_name,"ttyACM"))
                                   				//{ 
										if(NULL != strstr(cbInterfaceNumber,Devices[i].modem.number)) 
										{
											snprintf(SERIAL_PORT_MODEM_KEYNAME, sizeof(SERIAL_PORT_MODEM_KEYNAME)-1, "/dev/%s", pNext1->d_name); //coverity=SECURE_CODING
											LOGD("modem port is %s\n",SERIAL_PORT_MODEM_KEYNAME);
											ret |= 1 << 0 ;//modify by kf33986 DTS2010102503678 �����USB����������tyyUSB�豸û����ȫ�����Ͳ��Ҷ˿ڣ����¶˿��޷���
										}
										else if(NULL != strstr(cbInterfaceNumber,Devices[i].diag.number))
										{
											snprintf(SERIAL_PORT_DIAG_KEYNAME, sizeof(SERIAL_PORT_DIAG_KEYNAME)-1, "/dev/%s", pNext1->d_name);  //coverity=SECURE_CODING
											LOGD("diag port is %s\n",SERIAL_PORT_DIAG_KEYNAME);
											ret |= 1 << 1 ;//modify by kf33986 DTS2010102503678 �����USB����������tyyUSB�豸û����ȫ�����Ͳ��Ҷ˿ڣ����¶˿��޷���
										}
										else if(NULL != strstr(cbInterfaceNumber,Devices[i].pcui.number))
										{
											snprintf(SERIAL_PORT_PCUI_KEYNAME, sizeof(SERIAL_PORT_PCUI_KEYNAME)-1, "/dev/%s", pNext1->d_name);  //coverity=SECURE_CODING
											LOGD("pcui port is %s\n",SERIAL_PORT_PCUI_KEYNAME);
											ret |= 1 << 2 ;//modify by kf33986 DTS2010102503678 �����USB����������tyyUSB�豸û����ȫ�����Ͳ��Ҷ˿ڣ����¶˿��޷���
										}
										/*END DTS2013091610549 w84014221 2013-09-21 modified*/
									//}
								//}
			/* BEGIN DTS2011123100128  lkf52201 2011-12-31 added */
								//if (NULL != pDir2)
								//{
								    //closedir(pDir2);
								//}
							}	
						}
						if (NULL != pDir1)
						{
						    closedir(pDir1);
						}
						fclose(pfbInterfaceNumber);
					}
				}				
			}
			fclose(pfmodalias);
			/* END DTS2011123100128  lkf52201 2011-12-31 added */
		}		
	}
	/*END DTS2013083105452 w84014221 2013-10-26 for modified*/

	/* BEGIN DTS2011123100128  lkf52201 2011-12-31 modified */
	if (NULL != pDir_acm)
	{
	    closedir(pDir_acm);
	}
	/* END DTS2011123100128  lkf52201 2011-12-31 modified */
	return ret;
}