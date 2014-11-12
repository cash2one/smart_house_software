/*!
 * \file QrobotController.h
 * \brief 定义QrobotController类
 */

#ifndef QROBOT_CONTROLLER_H
#define QROBOT_CONTROLLER_H

namespace qrobot {

class Motion;

//! 机器人事件枚举值
enum QrobotMsg
{
    POWER_OFF = 0,          /*!< 断电 */
    MOTOR_POSITION = 1,     /*!< 头位置 */
    HEAD_TOUCH_DOWN = 2,    /*!< 触摸头 */
    HEAD_TOUCH_UP = 3,      /*!< 放开头 */
    HEAD_LONG_TOUCH = 4,    /*!< 长按头 */
    NECK_TOUCH_DOWN = 5,    /*!< 触摸下巴 */
    NECK_TOUCH_UP = 6,      /*!< 放开下巴 */
    UNKNOWN = -1            /*!< 未知 */
};

//! QrobotController类
/*! 
 * 提供小Q机器人控制接口
 */
class QrobotController
{
public:
    //! 获取QrobotController对象
	static QrobotController& Instance() 
    {
        static QrobotController controller;
        return controller;
    }

private:
    //! 构造函数
    /*!
     * 实现单例模式，构造函数定义为私有成员
     */
    QrobotController();
    virtual ~QrobotController();
    QrobotController(const QrobotController&) {}
    QrobotController& operator = (const QrobotController&)
    {
        return *this;
    }
public:
    //! 重置机器人动作
    bool Reset();
	
    //! 控制左翅膀运动，停止时位置偏上
    /*!
     *  \param speed 运动速度档位 (1 2 3)
     *  \param time 运动时间 (0-100) 单位为20ms
     */
    bool LeftWingUp(int speed, int time);

    //! 控制左翅膀运动，停止时位置偏下
    /*!
     *  \param speed 运动速度档位 (1 2 3)
     *  \param time 运动时间 (0-100) 单位为20ms
     */
    bool LeftWingDown(int speed, int time);
    
    //! 控制左翅膀运动回到原点
    /*!
     *  \param speed 运动速度档位 (1 2 3)
     */
    bool LeftWingOrg(int speed);
   
    //! 控制右翅膀运动，停止时位置偏上
    /*!
     *  \param speed 运动速度档位 (1 2 3)
     *  \param time 运动时间 (0-100) 单位为20ms
     */
    bool RightWingUp(int speed, int time);
    
    //! 控制右翅膀运动，停止时位置偏下
    /*!
     *  \param speed 运动速度档位 (1 2 3)
     *  \param time 运动时间 (0-100) 单位为20ms
     */
    bool RightWingDown(int speed, int time);
    
    //! 控制右翅膀运动回到原点
    /*!
     *  \param speed 运动速度档位 (1 2 3)
     */
    bool RightWingOrg(int speed);
    
    //! 控制头部水平运动
    /*!
     *  \param speed 运动速度档位(1 2 3)
     *  \param range 运动角度 (-80~80) 相对于原点
     */
    bool HorizontalHead(int speed, int range);

    //! 控制头部水平运动回原点
    /*!
     *  \param speed 运动速度档位(1 2 3)
     */
    bool HorizontalHeadOrg(int speed);
    
    //! 控制头部垂直运动
    /*!
     *  \param speed 运动速度档位(1 2 3)
     *  \param range 运动角度 (0~40) 相对于原点
     */
    bool VerticalHead(int speed, int range);
    
    //! 控制头部垂直运动回原点
    /*!
     *  \param speed 运动速度档位(1 2 3)
     */
    bool VerticalHeadOrg(int speed);
 
    //! 设置心脏颜色
    /*!
     *  \param r RGB值
     *  \param g RGB值
     *  \param b RGB值
     */
    bool Heart(int r, int g, int b);
    
    //! 设置眼睛表情
    /*!
     * \param time 表情循环次数
     * \param face 表情编号，详见Qrobot运动脚本编辑文档
     */
    bool Eye(int time, int face);

    //! 设置机器人电源状态
    void SetPowerState(bool state)
    {
        powerstate_ = state;
    }
   
    //! 向小Q读取数据
    /*!
    * \param waittime 在waittime时间内等待数据
    * \return 表示读取到数据的信息(-1：未知，0:断电，1：读取到了头部位置信息，2：头部触摸DOWN，3：头部触摸UP，4：头部触摸长按，5：脖子触摸DOWN，6：脖子触摸UP)
    */
    int Query(int waittime);

    //! 读取头部位置
    /*!
    * \return int[0]表示头部水平位置，int[1]表示头部垂直位置
    */
   int* GetHeadPosition();

private:

    //! 头部位置水平角度 取值：-80～80
	int hheadagl;

    //! 头部位置水平角度 取值：0～40
	int vheadagl;
	
    //! usb_handler
	struct  libusb_device_handle *usb_handle_;

    //! 电源状态
    bool powerstate_;

    //! 清理usb handler
    void Clear();

    //! 发送动作指令至机器人
    /*!
     * \param motion Motion类对象
     */
    bool SendMotionCtrl(Motion &motion);

    //! 解析机器人发回的数据消息
    /*!
     * \return 时间消息枚举值
     * \param data 消息数据buffer
     */
    QrobotMsg ParseData(unsigned char *data);

    //! 读取usb端口数据
    /*!
     * \param endpoint usb端口 （1 2）
     * \param rcv_buf 接收数据buffer
     * \param read_len 接受数据长度
     * \param wait_time 阻塞时间
     */
    int ReadData(unsigned char endpoint, unsigned char *rcv_buf,
		int read_len, int wait_time);
    
    //! 发送数据至usb端口
    /*!
     * \param endpoint usb端口 （1 2）
     * \param send_buf 发送数据buffer
     * \param send_len 发送数据长度
     * \param wait_time 阻塞时间
     */
    int WriteData(unsigned char endpoint, unsigned char *send_buf,
		int write_len, int wait_time);
};

} // end of qrobot

#endif // QROBOTCONTROLLER_H
