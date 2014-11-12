#include <thread>
#include "QrobotController.h"
#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <alsa/asoundlib.h>
#include <zmq.h>
#include <unistd.h>
#include <assert.h>
#include <time.h>
// second file///
#include "/usr/include/python2.7/Python.h"
#include <string>

// third file ///
#include <string.h>
#include <unistd.h>
#include <cstring>
#include "../include/qisr.h"
#include "../include/msp_cmn.h"
#include "../include/msp_errors.h"

// 4th file///
#include "../include/qtts.h"

typedef int SR_DWORD;
typedef short int SR_WORD ;

//音频头部格式
struct wave_pcm_hdr
{
	char            riff[4];                        // = "RIFF"
	SR_DWORD        size_8;                         // = FileSize - 8
	char            wave[4];                        // = "WAVE"
	char            fmt[4];                         // = "fmt "
	SR_DWORD        dwFmtSize;                      // = 下一个结构体的大小 : 16

	SR_WORD         format_tag;              // = PCM : 1
	SR_WORD         channels;                       // = 通道数 : 1
	SR_DWORD        samples_per_sec;        // = 采样率 : 8000 | 6000 | 11025 | 16000
	SR_DWORD        avg_bytes_per_sec;      // = 每秒字节数 : dwSamplesPerSec * wBitsPerSample / 8
	SR_WORD         block_align;            // = 每采样点字节数 : wBitsPerSample / 8
	SR_WORD         bits_per_sample;         // = 量化比特数: 8 | 16

	char            data[4];                        // = "data";
	SR_DWORD        data_size;                // = 纯数据长度 : FileSize - 44 
} ;

//默认音频头部数据
struct wave_pcm_hdr default_pcmwavhdr = 
{
	{ 'R', 'I', 'F', 'F' },
	0,
	{'W', 'A', 'V', 'E'},
	{'f', 'm', 't', ' '},
	16,
	1,
	1,
	16000,
	32000,
	2,
	16,
	{'d', 'a', 't', 'a'},
	0  
};

using namespace qrobot;
using namespace std;

//void test_func(int i)
//{
// // do something
//  cout<<"In thread.\n i="<<i<<endl;
//}

/*函数声明的地方*/
void playback(char* filename);
void record(char* filename);
int text_to_speech(const char* src_text ,const char* des_path ,const char* params);
int tts(char * input, int index);//index 1
void run_iat(const char* src_wav_filename ,  const char* param);
int iat();
char * Py_func_for_string(string filename, string methodname);
bool setModeOn(int mode);
bool setModeOff();
void selfCon(int mode);

/*thread function for sensor*/
void sensor_temp (void* responder);
void sensor_bright( void* responder);
void conTV ();

/*global var*/
//char socket_tel[5] = "6001";  //television
//char socket_lig[5] = "6002";  //light
//char socket_win[5] = "6003";  //window
//char socket_door[5] = "6004"; //door
//char socket_con[5] = "6005";  //air condition
//char socket_sensor[5] = "5555"  //sensor

bool self_control = true;  
bool off = true;
int temp;  //temperature
int bright;  //bright    
int month , day , hour , minute , second;
////////////////////////////////////////////////////////////////////////////////////////
 //        /\			         /\
 //       /  \		            /  \
 //      /----\		           /----\
 //     /      \		      /      \
 //    /        \		     /        \
////////////////////////////////////////////////////////////////////////////////////////
/*主函数开始的地方*/
int main(){
	QrobotController &controller = QrobotController::Instance();  //利用QrobotController库控制机器人的动作和表情，还能获取头部和颈部的传感器信息
	// execute thread
	//int i=99;
  	//thread mThread( test_func, i);
	//python initial
	 Py_Initialize();  //创建Python解析器

/////////////////////////////////////////////////////////
    /*初始化分词模块*/
    PyRun_SimpleString("import sys");
    PyRun_SimpleString("sys.path.append('./')");
    cout<<"ok?"<<endl;
    PyObject * pModule = NULL;
    PyObject * pFunc = NULL;
    string fn = "simpleJson";
    string mn = "fenci_initalize";
    pModule = PyImport_ImportModule(fn.c_str());
    if(pModule == NULL){
      cout << "NULL" <<endl;}
    pFunc = PyObject_GetAttrString(pModule,mn.c_str());
    
    PyObject * pyvalue;
    pyvalue = PyObject_CallObject(pFunc,NULL);

////////////////////////////////////////////////////////

		//初始化传感器的线程
		//温度
		void *context = zmq_ctx_new ();
    	void *responder = zmq_socket (context, ZMQ_REP);
    	int rc = zmq_bind (responder, "tcp://*:5555");
		assert (rc == 0);
		thread pthread1(sensor_temp,responder);
		sleep(1);
        pthread1.detach();
        //光照
        void *context2 = zmq_ctx_new ();
    	void *responder2 = zmq_socket (context2, ZMQ_REP);
    	int rc2 = zmq_bind (responder2, "tcp://*:6000");
	 	assert (rc2 == 0);
	 	thread pthread2(sensor_bright,responder2);
	 	sleep(1);
        pthread2.detach();
///////////////////////////////////////////////////////// 
	//机器人一开始的欢迎动作
	controller.HorizontalHead(2, -80);
	controller.LeftWingUp(2,10);
	controller.RightWingUp(2,10);
	sleep(1);
	controller.HorizontalHead(2, 80);
	controller.LeftWingDown(2,10);
	controller.RightWingDown(2,10);
	sleep(1);
	controller.HorizontalHead(2, 0);
	sleep(1.5);

	int* headpos = controller.GetHeadPosition();
	printf("Head Horizon Position : %d\n", headpos[0]);
	printf("Head Vertical Position : %d\n", headpos[1]);

	if(headpos[0]<(-10)||headpos[0]>(10)) 
	{controller.Reset();
	 cout<<"Reset!!!"<<endl;
	}
	 
	//触摸事件
	int query = -1;  //QrobotController的传入事件
	int i;
	int n ;
	bool k=true;
	char filename[20] = "test.wav";  //录制语音的文件名
	string filename2 = "simpleJson"; //python文件名
    string methodname = "makeDecision"; //python函数
    char * text;  //生成字符串
	//设置开机表情
	controller.Eye(1,31);
	sleep(1);
	/////////////////////////////////////////////////////////////////cjs
	//随机播放欢迎的语音
	srand((unsigned int) time(0));
	int num = rand() % 5;  //five pcm to play out
	switch(num)
	{
		case 0 :
			playback("1_1.pcm");break;
		case 1 :
			playback("1_2.pcm");break;
		case 2 :
			playback("1_3.pcm");break;
		case 3 :
			playback("1_4.pcm");break;
		case 4 :
			playback("1_5.pcm");break;
		default:
			printf("error from srand!");
	}
	/////////////////////////////////////////////////////////////////cjs
	//机器人进入等待触发的状态
	while(1){
		
		n = 99;  //每次都要重置作为分支标识的n值
		query = controller.Query(0); //获取从机器人传进来的触发信号
		switch(query){
		case 0:     //断电信号
			printf("power off ~\n");
			sleep(1);
 			break;
		case 1:     //头部位置发生变化的信号
			printf("head position ~\n");
			sleep(1);
			break;
		case 2:     //头部按下的信号
			printf("Touch Head Down\n");
			controller.Eye(1,16);
			//i = record(filename);
			//sleep(1);
			break; 		
		case 3:     //头部按下松开的信号
			printf("Touch Head Up\n");
			sleep(1);
			controller.Eye(1,36);  //录制语音的表情
			record(filename);  //录制声音为wav文件，并播放“请稍候”
			/*开始处理*/
			controller.Eye(10,3);  //正在处理的表情 刷新10次
			iat();//语音生成json文件
            //cout<<"ok?"<<endl;
			//把json文件的信息进行分类处理，导向不同的分支，返回包含分支标识或上网搜索到的答案的字符数组
    		text = Py_func_for_string(filename2,methodname);
            cout<<"|ok?"<<endl;
            cout<<text<<endl;
			//从字符数组中提取出需要的信息（如：分支标识/月/日/时/分）
			sscanf(text, "%d %d月%d日 %d:%d ",&n , &month , &day , &hour , &minute);
            cout<<n<<endl;
		

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    		if(n == 8) //分支标识8（定时开启电视机功能）
    		{
    			thread pthread(conTV);
				printf("将预定于%d月%d日 %d:%d 打开电视\n", month , day , hour , minute);
				sleep(1);
				pthread.detach();
				sleep(1);
    			controller.Eye(1,16);
    			int num1 = rand() % 2;  //随机播放“完成任务”的语音
				switch(num1)
				{
				case 0 :
					playback("ming.pcm");break;
				case 1 :
					playback("ming2.pcm");break;
				
				default:
					printf("error from srand!");
				}

    		}
			else if(n == 1)  //开启智能家居模式1——外出
    		{
    			k=setModeOn(1);
    			controller.Eye(1,16);
				if(k)
    				playback("mode_1.pcm");
    		}
    		else if(n == 2)  //开启智能家居模式2——睡眠
    		{
    			k=setModeOn(2);
    			controller.Eye(1,16);
				if(k)
    				playback("mode_2.pcm");
    		}
    		else if(n == 3)  //开启智能家居模式3——活动
    		{
    			k=setModeOn(3);
    			controller.Eye(1,16);
				if(k)
    				playback("mode_3.pcm");
    		}
    		else if(n == -1) //关闭智能家居模式1——外出
    		{
    			setModeOff();
    			controller.Eye(1,16);
    			playback("mode_g1.pcm");
    			
    		}
    		else if(n == -2) //关闭智能家居模式2——睡眠
    		{
    			setModeOff();
    			controller.Eye(1,16);
    			playback("mode_g2.pcm");
    			
    		}
    		else if(n == -3) //关闭智能家居模式3——活动
    		{
    			setModeOff();
    			controller.Eye(1,16);
    			playback("mode_g3.pcm");
    			
    		}
    		else if(strcmp(text , "开灯") == 0 || strcmp(text , "关灯") == 0|| strcmp(text , "开空调") == 0||strcmp(text , "关空调") == 0||
			strcmp(text , "开门") == 0||strcmp(text , "关门") == 0||strcmp(text , "开窗帘") == 0||strcmp(text , "关窗帘") == 0||
			strcmp(text , "开电视") == 0||strcmp(text , "关电视") == 0)  //语音命令控制家居（会自动关闭后台正运行的智能家居模式）
    		{
    			setModeOff();
    			controller.Eye(1,16);
    			//srand((unsigned int) time(0));
				//随机播放“完成任务”的语音
				int num1 = rand() % 2;  
				switch(num1)
				{
				case 0 :
					playback("ming.pcm");break;
				case 1 :
					playback("ming2.pcm");break;
				
				default:
					printf("error from srand!");
				}
    			
    		}
			/*else if(n == 0)  
    		{
    			controller.Eye(1,17);
    			playback("bye.pcm");
    			break;

    		}*/
    		else  //若不能匹配任何的分支，就直接把text字符数组的内容转换为语音文件并播放
    		{
    			tts(text,1);//生成pcm语音文件 文件名 file1.pcm
				/*播放刚生成的语音文件*/
				controller.Eye(1,16);
				playback("file1.pcm");
    		}
			
			sleep(1);
			break;

////////////////////////////////////////////////////////////////////////////////////////////////////////////
		case 4:     //长按头部的信号
			printf("Bye Bye ~\n");
			controller.Eye(1,32);
			sleep(1);
			exit(0);
			break;
		case 5:     //触碰颈部的信号
			printf("neck touch ~\n");
			controller.Eye(1,16);
			playback("file1.pcm");
			sleep(1);		
			break;
		case 6:     //颈部触碰放松的信号
			printf("neck up ~\n");
			controller.Eye(1,41);
			sleep(1);		
			break;
		case 7:     //未知信号
			printf("unknown ~\n");
			sleep(1);		
			break;

		default:    //错误信号
			printf("error fff\n");
			sleep(1);
		}
	}
	Py_Finalize();      //调用Py_Finalize，这个根Py_Initialize相对应的。
	/*关闭温度和光照传感器的zeromq环境和对端口的绑定*/
	zmq_close (responder);   
    zmq_ctx_destroy (context);
	zmq_close (responder2);
    zmq_ctx_destroy (context2);
	return 0;
}

/*record sound*/
void record(char* filename)
{
	int i;
	int ret;
	int buf[128];
	unsigned int val;
    int dir=0;
	char *buffer;
	int size;
	snd_pcm_uframes_t frames;
    snd_pcm_uframes_t periodsize;
	snd_pcm_t *playback_handle;//PCM设备句柄pcm.h
	snd_pcm_hw_params_t *hw_params;//硬件信息和PCM流配置
	printf("record into file %s by wolf\n", filename);
	/*打开文件*/
	FILE *fp = fopen(filename, "wb");
    if(fp == NULL)
    	return;
    fseek(fp, 100, SEEK_SET);//跳过.wav文件的头信息
	
	//1. 打开PCM，最后一个参数为0意味着标准配置
	ret = snd_pcm_open(&playback_handle, "default", SND_PCM_STREAM_CAPTURE, 0);
	if (ret < 0) {
		perror("snd_pcm_open");
		exit(1);
	}
	
	//2. 分配snd_pcm_hw_params_t结构体
	ret = snd_pcm_hw_params_malloc(&hw_params);
	if (ret < 0) {
		perror("snd_pcm_hw_params_malloc");
		exit(1);
	}
	//3. 初始化hw_params
	ret = snd_pcm_hw_params_any(playback_handle, hw_params);
	if (ret < 0) {
		perror("snd_pcm_hw_params_any");
		exit(1);
	}
	//4. 初始化访问权限
	ret = snd_pcm_hw_params_set_access(playback_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
	if (ret < 0) {
		perror("snd_pcm_hw_params_set_access");
		exit(1);
	}
	//5. 初始化采样格式SND_PCM_FORMAT_U8,8位   16wei
	ret = snd_pcm_hw_params_set_format(playback_handle, hw_params, SND_PCM_FORMAT_S16_LE);
	if (ret < 0) {
		perror("snd_pcm_hw_params_set_format");
		exit(1);
	}
	//6. 设置采样率，如果硬件不支持我们设置的采样率，将使用最接近的
	//val = 44100,有些录音采样频率固定为8KHz val = 16000
	

	val = 16000;
	ret = snd_pcm_hw_params_set_rate_near(playback_handle, hw_params, &val, &dir);
	if (ret < 0) {
		perror("snd_pcm_hw_params_set_rate_near");
		exit(1);
	}
	//7. 设置通道数量
	ret = snd_pcm_hw_params_set_channels(playback_handle, hw_params, 1);
	if (ret < 0) {
		perror("snd_pcm_hw_params_set_channels");
		exit(1);
	}
	
    /* Set period size to 32 frames. */
    frames = 32;
    periodsize = frames * 2;
    ret = snd_pcm_hw_params_set_buffer_size_near(playback_handle, hw_params, &periodsize);
    if (ret < 0) 
    {
         printf("Unable to set buffer size %li : %s\n", frames * 2, snd_strerror(ret));
         
    }
          periodsize /= 2;

    ret = snd_pcm_hw_params_set_period_size_near(playback_handle, hw_params, &periodsize, 0);
    if (ret < 0) 
    {
        printf("Unable to set period size %li : %s\n", periodsize,  snd_strerror(ret));
    }
								  
	//8. 设置hw_params
	ret = snd_pcm_hw_params(playback_handle, hw_params);
	if (ret < 0) {
		perror("snd_pcm_hw_params");
		exit(1);
	}
	
    /* Use a buffer large enough to hold one period */
    snd_pcm_hw_params_get_period_size(hw_params, &frames, &dir);
                                
    size = frames * 2; /* 2 bytes/sample, 2 channels */
    buffer = (char *) malloc(size);
    fprintf(stderr,
            "size = %d\n",
            size);
    int j = 0;
    while (1) 
    {
		//9. 读PCM设备捕获的数据到buffer中
        while(ret = snd_pcm_readi(playback_handle, buffer, frames)<0)
        {
            usleep(1);
            if (ret == -EPIPE)
            {
                  /* EPIPE means underrun */
                  fprintf(stderr, "overrun occurred\n");
				  //完成硬件参数设置，使设备准备好
                  snd_pcm_prepare(playback_handle);
            } 
            else if (ret < 0) 
            {
                  fprintf(stderr,"error from writei: %s\n",snd_strerror(ret));
            }  
        }

 		//写buffer中数据到文件中
		ret = fwrite(buffer, 1, size, fp);
        j++;
	     // j为计数器，当计数器到了1600之后，退出循环，现实中大概有4.5s的录音时间
        if(j > 1600) 
		{
        	fprintf(stderr, "end of record\n");
        	break;
        }
    }		
	//10. 关闭PCM设备句柄 并 free memory
	snd_pcm_close(playback_handle);
	fclose(fp);
	free(buffer);
	buffer=NULL;
	
	/*创建播放“请稍候”的线程*/
	char waitname1[20]="wait1.pcm";//“等待后台处理”的播放文件名
	char waitname2[20]="r1.pcm";//“等待后台处理”的播放文件名
	char waitname3[20]="r2.pcm";//“等待后台处理”的播放文件名
	char* waitname;
	int num2 = rand() % 3;  // 随机播放“请稍候”的语音
				switch(num2)
				{
				case 0 :
					waitname = waitname1;break;
				case 1 :
					waitname = waitname2;break;
				case 2 :
					waitname = waitname3;break;
				default:
					printf("error from srand!");
				}
	
	thread mThread(playback, waitname);
	sleep(1);
	mThread.detach();
	return;
}

/*playback sound*/
void playback(char* filename)
{
	int i;
	int ret;
	int buf[128];
	unsigned int val;
    int dir=0;
	char *buffer;
	int size;
	snd_pcm_uframes_t frames;
   	snd_pcm_uframes_t periodsize;
	snd_pcm_t *playback_handle;//PCM设备句柄pcm.h
	snd_pcm_hw_params_t *hw_params;//硬件信息和PCM流配置
	//cout<< filename<<endl;
	printf("play song %s by wolf\n", filename);
	/*打开文件*/
	FILE *fp = fopen(filename, "rb");
   	if(fp == NULL)
    	return;
    fseek(fp, 100, SEEK_SET);//跳过.wav文件的头信息
	
	//1. 打开PCM，最后一个参数为0意味着标准配置
	ret = snd_pcm_open(&playback_handle, "default", SND_PCM_STREAM_PLAYBACK, 0);
	if (ret < 0) {
		perror("snd_pcm_open");
		exit(1);
	}
	
	//2. 分配snd_pcm_hw_params_t结构体
	ret = snd_pcm_hw_params_malloc(&hw_params);
	if (ret < 0) {
		perror("snd_pcm_hw_params_malloc");
		exit(1);
	}
	//3. 初始化hw_params
	ret = snd_pcm_hw_params_any(playback_handle, hw_params);
	if (ret < 0) {
		perror("snd_pcm_hw_params_any");
		exit(1);
	}
	//4. 初始化访问权限
	ret = snd_pcm_hw_params_set_access(playback_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
	if (ret < 0) {
		perror("snd_pcm_hw_params_set_access");
		exit(1);
	}
	//5. 初始化采样格式SND_PCM_FORMAT_U8,8位   16wei
	ret = snd_pcm_hw_params_set_format(playback_handle, hw_params, SND_PCM_FORMAT_S16_LE);
	if (ret < 0) {
		perror("snd_pcm_hw_params_set_format");
		exit(1);
	}
	//6. 设置采样率，如果硬件不支持我们设置的采样率，将使用最接近的
	//val = 44100,有些录音采样频率固定为8KHz val = 16000
	

	val = 16000;
	ret = snd_pcm_hw_params_set_rate_near(playback_handle, hw_params, &val, &dir);
	if (ret < 0) {
		perror("snd_pcm_hw_params_set_rate_near");
		exit(1);
	}
	//7. 设置通道数量
	ret = snd_pcm_hw_params_set_channels(playback_handle, hw_params, 1);
	if (ret < 0) {
		perror("snd_pcm_hw_params_set_channels");
		exit(1);
	}
	
    /* Set period size to 32 frames. */
    frames = 32;
    periodsize = frames * 2;
    ret = snd_pcm_hw_params_set_buffer_size_near(playback_handle, hw_params, &periodsize);
    if (ret < 0) 
    {
         printf("Unable to set buffer size %li : %s\n", frames * 2, snd_strerror(ret));
         
    }
          periodsize /= 2;

    ret = snd_pcm_hw_params_set_period_size_near(playback_handle, hw_params, &periodsize, 0);
    if (ret < 0) 
    {
        printf("Unable to set period size %li : %s\n", periodsize,  snd_strerror(ret));
    }
								  
	//8. 设置hw_params
	ret = snd_pcm_hw_params(playback_handle, hw_params);
	if (ret < 0) {
		perror("snd_pcm_hw_params");
		exit(1);
	}
	
    /* Use a buffer large enough to hold one period */
    snd_pcm_hw_params_get_period_size(hw_params, &frames, &dir);
                                
    size = frames * 2; /* 2 bytes/sample, 2 channels */
    buffer = (char *) malloc(size);
    fprintf(stderr,
            "size = %d\n",
            size);
    
    while (1) 
    {
        ret = fread(buffer, 1, size, fp);
        if(ret == 0)  //不能再从文件中提取到信息，即已到文件尾，因此退出循环
        {
              fprintf(stderr, "end of playback\n");
              break;
        } 
        else if (ret != size) 
        {
		fprintf(stderr, "error occur\n");
        }
	//9. 写音频数据到PCM设备
	//fprintf(stderr, "ret = %d\n",ret);
        while(ret = snd_pcm_writei(playback_handle, buffer, frames)<0)
        {
            usleep(1);
            if (ret == -EPIPE)
            {
                  /* EPIPE means underrun */
                  fprintf(stderr, "underrun occurred\n");
				  //完成硬件参数设置，使设备准备好
                  snd_pcm_prepare(playback_handle);
            } 
            else if (ret < 0) 
            {
                  fprintf(stderr,
                  "error from writei: %s\n",
                  snd_strerror(ret));
            }  
        }
        
    }		
	//10. 关闭PCM设备句柄 并 free memory
	snd_pcm_close(playback_handle);
	free(buffer);
	buffer=NULL;
	return;
}





char * Py_func_for_string(string filename, string methodname){
    PyRun_SimpleString("import sys");
    PyRun_SimpleString("sys.path.append('./')");
    cout<<"ok?"<<endl;
    PyObject * pModule = NULL;
    PyObject * pFunc = NULL;
    cout << filename << endl;
    
    pModule = PyImport_ImportModule(filename.c_str());
    if(pModule == NULL){
      cout << "NULL" <<endl;}
    pFunc = PyObject_GetAttrString(pModule,methodname.c_str());
    
    PyObject * pyvalue;
    pyvalue = PyObject_CallObject(pFunc,NULL);
    char* result;
    PyArg_Parse(pyvalue,"s",&result);
    return result;
    
}



void run_iat(const char* src_wav_filename ,  const char* param)
{
	char rec_result[1024*4] = {0};
	const char *sessionID;
	FILE *f_pcm = NULL;
	char *pPCM = NULL;
	int lastAudio = 0 ;
	int audStat = 2 ;
	int epStatus = 0;
	int recStatus = 0 ;
	long pcmCount = 0;
	long pcmSize = 0;
	int ret = 0 ;
	
	sessionID = QISRSessionBegin(NULL, param, &ret);
    if (ret !=0)
	{
	    printf("QISRSessionBegin Failed,ret=%d\n",ret);
	}
	f_pcm = fopen(src_wav_filename, "rb");
	if (NULL != f_pcm) {
		fseek(f_pcm, 0, SEEK_END);
		pcmSize = ftell(f_pcm);
		fseek(f_pcm, 0, SEEK_SET);
		pPCM = (char *)malloc(pcmSize);
		fread((void *)pPCM, pcmSize, 1, f_pcm);
		fclose(f_pcm);
		f_pcm = NULL;
	}
	while (1) {
		unsigned int len = 6400;

		if (pcmSize < 12800) {
			len = pcmSize;
			lastAudio = 1;
		}
		audStat = 2;
		if (pcmCount == 0)
			audStat = 1;
		if (0) {
			if (audStat == 1)
				audStat = 5;
			else
				audStat = 4;
		}
		if (len<=0)
		{
			break;
		}
		printf("csid=%s,count=%d,aus=%d,",sessionID,pcmCount/len,audStat);
		ret = QISRAudioWrite(sessionID, (const void *)&pPCM[pcmCount], len, audStat, &epStatus, &recStatus);
		printf("eps=%d,rss=%d,ret=%d\n",epStatus,recStatus,ret);
		if (ret != 0)
			break;
		pcmCount += (long)len;
		pcmSize -= (long)len;
		if (recStatus == 0) {
			const char *rslt = QISRGetResult(sessionID, &recStatus, 0, &ret);
			if (ret != MSP_SUCCESS)
			{
				printf("QISRGetResult Failed,ret=%d\n",ret);
				break;
			}
			if (NULL != rslt)
				printf("%s\n", rslt);
		}
		if (epStatus == MSP_EP_AFTER_SPEECH)
			break;
		usleep(1);
	}
	ret=QISRAudioWrite(sessionID, (const void *)NULL, 0, 4, &epStatus, &recStatus);
	if (ret !=0)
	{
		printf("QISRAudioWrite Failed,ret=%d\n",ret);
	}
	free(pPCM);
	pPCM = NULL;
        printf("%d\n",recStatus);
	while (recStatus != 5 && ret == 0) {
		const char *rslt = QISRGetResult(sessionID, &recStatus, 0, &ret);
		if (NULL != rslt)
		{
			strcat(rec_result,rslt);
		}
		usleep(1);
	}
    ret=QISRSessionEnd(sessionID, NULL);
	if(ret !=MSP_SUCCESS)
	{
		printf("QISRSessionEnd Failed, ret=%d\n",ret);
	}

	printf("=============================================================\n");
	printf("The result is: %s\n",rec_result);
        printf("outputing!");
        FILE * fp;
        //string json = rec_result;
        if ((fp = fopen("test.json","w")) == NULL){
          printf("error");
          exit(0);
        }
        fputs( rec_result,fp);
        fclose(fp);
	printf("=============================================================\n");
	usleep(1);
}

int iat()
{
    ///APPID请勿随意改动
	const char* login_configs = "server_url=http://dev.voicecloud.cn:80/index.htm,appid=53a24a5a,work_dir =   .  ";
	const char* param1 = "sub=iat,ssm=1,auf=audio/L16;rate=16000,aue=speex,ent=sms16k,rst=plain,rse=gb2312";//直接转写，默认编码为gb2312，可以通过rse参数指定为utf8或unicode
	const char* param2 = "sub=iat,ssm=1,auf=audio/L16;rate=16000,aue=speex,ent=sms16k,rst=json,rse=utf8";//转写为json格式，编码只能为utf8
        const char* param3 = "sub=iat,ssm=1,sch=1,auf=audio/L16;rate=16000,aue=speex-wb;10,ent=sms16k,ptt=0,rst=json,rse=utf8,nlp_version=2.0,vad_timeout =30000 "; 
	const char* param4 = "sub=iat,ssm=1,sch=1,auf=audio/L16;rate=16000,aue=speex,ent=cantonese16k,ptt=0,rst=json,rse=utf8,nlp_version=2.0"; 
	const char* output_file = "iat_result.txt";
	int ret = 0;
	char key = 0;
    
	//用户登录
	ret = MSPLogin(NULL, NULL, login_configs);
	if ( ret != MSP_SUCCESS )
	{
		printf("MSPLogin failed , Error code %d.\n",ret);
	}
	//开始一路转写会话
	//run_iat("wav/iflytek09.wav" ,  param2);                                     //iflytek09对应的音频内容“沉舟侧畔千帆过，病树前头万木春。”
	run_iat("test.wav" ,  param3);                                     //iflytek01对应的音频内容“科大讯飞”
    
	//退出登录
       MSPLogout();
	return 0;
    
}

int text_to_speech(const char* src_text ,const char* des_path ,const char* params)
{
	struct wave_pcm_hdr pcmwavhdr = default_pcmwavhdr;
	const char* sess_id = NULL;
	int ret = 0;
	unsigned int text_len = 0;
	char* audio_data;
	unsigned int audio_len = 0;
	int synth_status = 1;
	FILE* fp = NULL;

	printf("begin to synth...\n");
	if (NULL == src_text || NULL == des_path)
	{
		printf("params is null!\n");
		return -1;
	}
	text_len = (unsigned int)strlen(src_text);
	fp = fopen(des_path,"wb");
	if (NULL == fp)
	{
		printf("open file %s error\n",des_path);
		return -1;
	}
	sess_id = QTTSSessionBegin(params, &ret);
	if ( ret != MSP_SUCCESS )
	{
		printf("QTTSSessionBegin: qtts begin session failed Error code %d.\n",ret);
		return ret;
	}

	ret = QTTSTextPut(sess_id, src_text, text_len, NULL );
	if ( ret != MSP_SUCCESS )
	{
		printf("QTTSTextPut: qtts put text failed Error code %d.\n",ret);
		QTTSSessionEnd(sess_id, "TextPutError");
		return ret;
	}
	fwrite(&pcmwavhdr, sizeof(pcmwavhdr) ,1, fp);
	while (1) 
	{
		const void *data = QTTSAudioGet(sess_id, &audio_len, &synth_status, &ret);
		if (NULL != data)
		{
		   fwrite(data, audio_len, 1, fp);
		   pcmwavhdr.data_size += audio_len;//修正pcm数据的大小
		}
		if (synth_status == 2 || ret != 0) 
		break;
	}

	//修正pcm文件头数据的大小
	pcmwavhdr.size_8 += pcmwavhdr.data_size + 36;

	//将修正过的数据写回文件头部
	fseek(fp, 4, 0);
	fwrite(&pcmwavhdr.size_8,sizeof(pcmwavhdr.size_8), 1, fp);
	fseek(fp, 40, 0);
	fwrite(&pcmwavhdr.data_size,sizeof(pcmwavhdr.data_size), 1, fp);
	fclose(fp);

	ret = QTTSSessionEnd(sess_id, NULL);
	if ( ret != MSP_SUCCESS )
	{
	printf("QTTSSessionEnd: qtts end failed Error code %d.\n",ret);
	}
	return ret;
}
int tts(char * input, int index){

    const char * login_configs = " appid = 53a24a5a, work_dir =   .  ";

    

    //确定输出文件名

    const char * text = input;

    char char_index[10];

    sprintf(char_index,"%d",index);

    //itoa(index,char_index,10);
    string i = char_index;

    string filename = "file"+i+".pcm";
    const char* param = "aue = speex-wb;3, vcn=xiaoyan, aue=speex-wb;10, spd = 50, vol = 50, tte = utf8, rdn = 2";

int ret = 0;

char key = 0;

    

    //用户登录

ret = MSPLogin(NULL, NULL, login_configs);

if ( ret != MSP_SUCCESS )

{

printf("MSPLogin failed , Error code %d.\n",ret);

}

//音频合成

ret = text_to_speech(text,filename.c_str(),param);

if ( ret != MSP_SUCCESS )

{

printf("text_to_speech: failed , Error code %d.\n",ret);

}

//退出登录

    MSPLogout();

return 0;

    

    

}

/*thread function for temperature sensor*/
void sensor_temp (void* responder)
{
	while(1)
	{
  		char buffer [256];
    	int size = zmq_recv (responder, buffer, 255, 0);   // wait for the update from sensor
    	if (size == -1)
    	{
        	printf("recv error from sensor_temp function!!!\n");
        	return;
        }
    	if (size > 255)
    	{
        	size = 255;
        }
   		buffer [size] = 0;
        printf ("Receive %s\n",buffer);
		sscanf (buffer, "%d", &temp);
		printf ("temp = %d\n",temp);
        sleep (1);          //  Do some 'work'
        zmq_send (responder, "ok", 5, 0);
    }
    return;
}

/*thread function for bright sensor*/
void sensor_bright( void* responder)
{
	while(1)
	{
  		char buffer [256];
    	int size = zmq_recv (responder, buffer, 255, 0);   // wait for the update from sensor
    	if (size == -1)
    	{
        	printf("recv error from sensor_bright function!!!\n");
        	return;
        }
    	if (size > 255)
    	{
        	size = 255;
        }
   		buffer [size] = 0;
        printf ("Receive %s\n",buffer);
		sscanf (buffer, "%d", &bright);
		printf ("bright = %d\n",bright);
        sleep (1);          //  Do some 'work'
        zmq_send (responder, "ok", 5, 0);
    }
    return;
}

/*self-control mode on*/
bool setModeOn(int mode)
{
	if(!off)
	{
		printf("self-control mode is still turning on!! Please turn off it first !!\n");
		return false;
	}
	thread thread_server (selfCon, mode);
	printf("self-control mode is turning on!! \n");
	sleep(1); 
	thread_server.detach();
	return true;
}

/*self-control mode off*/
bool setModeOff()
{
	self_control=false;
	sleep(2);
	if(!off) 
	{
		printf("self-control mode is turning on!! Please turn off it first !!\n");
		return false;
	}
	return true;
}

void selfCon(int mode)
{	
	self_control=true;
	off = false;
	while(1)
	{
    	/*随时断开模式*/
		if(!self_control)
    	{
    		off = true;
    		printf("the self_control mode is turning off !!!\n");
    		break;
    	}

    	
    	char s[5] = {' ',' ',' ',' ',' '};
    	switch(mode)
    	{case 1 : //外出模式
    		s[0] = '0';  //television
    		s[1] = '0';  //light
    		s[2] = '0';  //window
    		s[3] = '0';  //door
    		s[4] = '0';  //air-condition
    		break;
    	 case 2 : //睡眠
    		s[0] = '0';
    		s[1] = '0';
    		s[2] = '0';
    		s[3] = '0';
    		//s[4] = '0';
    		if(temp>29)
    		{s[4] = '1';}
    		break;
    	 case 3 : //活动模式
    		if(bright==0)
    		{s[1] = '1';}
    		s[2] = '1';
    		s[3] = '0';
    		if(temp>29)
    		{s[4] = '1';}
    		break;
    	 default :
		    printf("switch case error in selfCon funciton!!!!\n");
    	}

    	//air-condition controlling
    	void *context5 = zmq_ctx_new ();
    	void *requester5 = zmq_socket (context5, ZMQ_REQ);
    	zmq_connect (requester5, "tcp://localhost:6005");
    	
        char buffer [3]={' ',' ',' '};
        sprintf (buffer, "%c\0", s[4]);
        printf ("Sending %c to air-condition…\n", s[4]);
        zmq_send (requester5, buffer, 3, 0);
        zmq_recv (requester5, buffer, 3, 0);
        printf ("Received %s\n", buffer);
    	zmq_close (requester5);
    	zmq_ctx_destroy (context5);

    	/*随时断开模式*/
		if(!self_control)
    	{
    		off = true;
    		printf("the self_control mode is turning off !!!\n");
    		break;
    	}

    	//window controlling
    	void *context3 = zmq_ctx_new ();
    	void *requester3 = zmq_socket (context3, ZMQ_REQ);
    	zmq_connect (requester3, "tcp://localhost:6003");
    	
    //   char buffer [3];
        sprintf (buffer, "%c\0", s[2]);
        printf ("Sending %c to window…\n", s[2]);
        zmq_send (requester3, buffer, 3, 0);
        zmq_recv (requester3, buffer, 3, 0);
        printf ("Received %s\n", buffer);
    	zmq_close (requester3);
    	zmq_ctx_destroy (context3);

    	/*随时断开模式*/
		if(!self_control)
    	{
    		off = true;
    		printf("the self_control mode is turning off !!!\n");
    		break;
    	}

    	//light controlling
    	void *context2 = zmq_ctx_new ();
    	void *requester2 = zmq_socket (context2, ZMQ_REQ);
    	zmq_connect (requester2, "tcp://localhost:6002");
    	
        char buffer2 [3]={' ',' ',' '};
        sprintf (buffer2, "%c\0", s[1]);
        printf ("Sending %c to light…\n", s[1]);
        zmq_send (requester2, buffer2, 3, 0);
        zmq_recv (requester2, buffer2, 3, 0);
        printf ("Received %s\n", buffer2);
    	zmq_close (requester2);
    	zmq_ctx_destroy (context2);

    	/*随时断开模式*/
		if(!self_control)
    	{
    		off = true;
    		printf("the self_control mode is turning off !!!\n");
    		break;
    	}

    	//door controlling
    	sleep(2);  // After 2 seconds, the door will close automatically
    	void *context4 = zmq_ctx_new ();
    	void *requester4 = zmq_socket (context4, ZMQ_REQ);
    	zmq_connect (requester4, "tcp://localhost:6004");
    	
    //    char buffer [3];
        sprintf (buffer, "%c\0", s[3]);
        printf ("Sending %c to door…\n", s[3]);
        zmq_send (requester4, buffer, 3, 0);
        zmq_recv (requester4, buffer, 3, 0);
        printf ("Received %s\n", buffer);
    	zmq_close (requester4);
    	zmq_ctx_destroy (context4);

    	/*随时断开模式*/
		if(!self_control)
    	{
    		off = true;
    		printf("the self_control mode is turning off !!!\n");
    		break;
    	}

    	if(mode == 3) continue;   // no need to control television when mode == "active"

    	//television controlling
    	void *context1 = zmq_ctx_new ();
    	void *requester1 = zmq_socket (context1, ZMQ_REQ);
    	zmq_connect (requester1, "tcp://localhost:6001");
    	
    //    char buffer [3];
        sprintf (buffer, "%c\0", s[0]);
        printf ("Sending %c to television…\n", s[0]);
        zmq_send (requester1, buffer, 3, 0);
        zmq_recv (requester1, buffer, 3, 0);
        printf ("Received %s\n", buffer);
    	zmq_close (requester1);
    	zmq_ctx_destroy (context1);
    
    
	}
	return;
}

void conTV () //定时开启电视机的函数
{
	tm* s;
	time_t timep;
	int month_ , day_ , hour_ , minute_ , second_ ;

	while(1){
	time(&timep); //获取时间
	s = localtime(&timep);  //更改时间格式
	
	month_ = s->tm_mon+1;
	day_ = s->tm_mday;
	hour_ = s->tm_hour;
	minute_ = s->tm_min;
	second_ = s->tm_sec;

	if( month==month_ && day==day_ && hour==hour_ && minute==minute_ )
	{
		char command[50];
    	strcpy( command, "google-chrome http://v.youku.com/v_show/id_XNzM2NDE4MzI0.html" ); //打开chrome浏览器
    	system(command);
		sleep(2);
		playback("TVplay.pcm");
		cout<<"电视定时开启功能已完成！！"<<endl;
		break;
	}
	sleep(1);
	}
	return;
}
