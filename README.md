# WinPlay
Windows 实时音频处理模板类
// 
// Windows 实时音频处理模板类 v1.0
// 音频数据类型 PCM Waveform-Audio Data Format （MSDN ms-help://MS.VSCC.v90/MS.MSDNQTR.v90.chs/multimed/htm/_win32_devices_and_data_types.htm）
// 
// 模板类支持的类型 
//     类型             数据格式      最大值             最小值               中间值
//     __int16          16-bit PCM    32,767 (0x7FFF)    - 32,768 (0x8000)    0
//     unsigned __int8   8-bit PCM    255 (0xFF)         0                    128 (0x80) 
//
// 使用：定义好自己的处理函数，用SetProcessFunc()设置。
//     处理函数原型 void (*process_func_type)(Type *in, Type *out, uint32 nbytes, void *p_param)
//     in        指向 nbytes 字节的录音数据
//     out       指向播放缓冲区
//     p_param   用户实例数据，通过 SetProcessFunc() 传递
// 重要提示：
//        1. 由于采用 PING-PONG BUFFER 缓冲机制，用户处理函数必须在 （buffer_size_points_ / sample_rate_）s 时间内完成处理。
//
// TODO： 1. 思考：准备缓冲区、释放内存等操作可否放在回调函数里完成
//        2. 设备未成功打开时的错误原因提示（现在只能由返回值查MSDN）
//        3. 思考：这里用模板类来实现是否合适
//        4. 采样率，通道数，缓冲区大小设置函数，构造函数加上这些默认参数
//
