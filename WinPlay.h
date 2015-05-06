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
// 作者：何长青 
// 邮箱：494268393@qq.com
//
#ifndef _WIN_PLAY_H
#define _WIN_PLAY_H

#include <windows.h>
#include <mmsystem.h>
#include <vector>

template <class Type> class WinPlay {
public:
	typedef unsigned __int8   uint8;
    typedef __int16           int16;
	typedef unsigned __int32 uint32;

	typedef void (*process_func_type)(Type *in, Type *out, uint32 nbytes, void *p_param);

	WinPlay(uint32 nbuffer_points = 512, uint32 channel = 1, uint32 sample_rate = 44100);
	~WinPlay();

	// 设置用户音频处理函数
	void SetProcessFunc(process_func_type, void *p_parm);
	// 开始实时录音、播放
	void Start();
	// 停止实时录音、播放
	void Stop();

	// 当输入输出设备都正常打开时返回 true
	bool OK() { return (device_state_.wave_in_open_ok && device_state_.wave_out_open_ok); }
	
private:
	void Init();
	static void CALLBACK WaveInProc(HWAVEIN hwi,        
						     UINT uMsg,           
							 DWORD_PTR dwInstance,    
							 DWORD_PTR dwParam1,    
						     DWORD_PTR dwParam2); 

private:
	enum { PLAYING, STOP, CLOSE } state_;
	const uint32 sample_rate_; 
	const uint32 sample_bits_;
	const uint32 channel_;

	const uint32 buffer_size_points_;
	const uint32 buffer_size_bytes_;
	const uint32 wave_in_buffer_num_;
	const uint32 wave_out_buffer_num_;

    WAVEFORMATEX wavformat_;
	HWAVEIN h_wave_in_;
	HWAVEOUT h_wave_out_;

	process_func_type process_func_;
	void *p_param_;

	std::vector<WAVEHDR> wave_in_buffer_hdr_;

	WAVEHDR wave_out_wavhdr_;

	struct Buffer {
		Type *ptr;
		size_t n;
		size_t limit;
		Buffer() : ptr(NULL), n(0), limit(0) {}
	};
	std::vector<Buffer> wave_out_buffer_;

	uint32 wave_out_buffer_reader_;
	uint32 wave_out_buffer_writer_;
private:
	struct DeviceState {
		bool wave_in_open_ok;
		bool wave_out_open_ok;
		MMRESULT wave_in_open_result;
		MMRESULT wave_out_open_result;
		DeviceState() : wave_in_open_ok(false), wave_out_open_ok(false) {}
	} device_state_;
};

#include "WinPlayInl.h"

#endif // _WIN_PLAY_H
