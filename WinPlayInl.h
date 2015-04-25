#ifndef _WIN_PLAY_INL_H
#define _WIN_PLAY_INL_H

#include <vector>

template <class Type>
WinPlay<Type>::WinPlay(uint32 nbuffer_points, uint32 channel, uint32 sample_rate) 
                         : sample_rate_(sample_rate), sample_bits_(sizeof(Type) * 8),
                           channel_(channel), buffer_size_points_(nbuffer_points), 
						   buffer_size_bytes_(buffer_size_points_ * sizeof(Type) * channel_),
						   wave_in_buffer_num_(4), wave_out_buffer_num_(2),
						   wave_out_buffer_reader_(0), wave_out_buffer_writer_(1),
						   process_func_(NULL), state_(STOP)
{
	Init();
}

// 关闭输入输出设备，释放缓冲区
template <class Type>
WinPlay<Type>::~WinPlay()
{	
	state_ = STOP;  // this is important, or the waveInReset will block
	if (device_state_.wave_in_open_ok) {
		waveInReset(h_wave_in_);
	}
	for (std::vector<WAVEHDR>::size_type i = 0; i != wave_in_buffer_hdr_.size(); i++) {
		if (device_state_.wave_in_open_ok) {
			waveInUnprepareHeader(h_wave_in_, &wave_in_buffer_hdr_[i], sizeof(WAVEHDR));
		}
		delete [] wave_in_buffer_hdr_[i].lpData;
	}
	if (device_state_.wave_in_open_ok) {
		waveInClose(h_wave_in_); 
	}
	if (device_state_.wave_out_open_ok) {
 		waveOutReset(h_wave_out_);
		waveOutUnprepareHeader(h_wave_out_, &wave_out_wavhdr_, sizeof(WAVEHDR));
	}
	for (std::vector<Buffer>::size_type i = 0; i != wave_out_buffer_.size(); i++) {
		delete [] wave_out_buffer_[i].ptr;
	}
	if (device_state_.wave_out_open_ok) {
		waveOutClose(h_wave_out_);
	}
}

// 准备缓冲区，打开设备
template <class Type>
void WinPlay<Type>::Init()
{
	// 这里定义了音频基本参数
	wavformat_.wFormatTag = WAVE_FORMAT_PCM;  
	wavformat_.nChannels = channel_;  
	wavformat_.nSamplesPerSec = sample_rate_;  
	wavformat_.nAvgBytesPerSec = sample_rate_ * sample_bits_ * channel_ / 8;  
	wavformat_.nBlockAlign = sample_bits_ * channel_ / 8;  
	wavformat_.wBitsPerSample = sample_bits_;  
	wavformat_.cbSize = 0;  
	// 准备输出缓冲区，也就是 PING-PONG BUFFER
	wave_out_buffer_.resize(wave_out_buffer_num_);
	for (std::vector<Buffer>::size_type i = 0; i != wave_out_buffer_.size(); i++) {
		wave_out_buffer_[i].ptr   = new Type [buffer_size_points_ * channel_];
		wave_out_buffer_[i].n     = buffer_size_bytes_;
		wave_out_buffer_[i].limit = buffer_size_bytes_;
	}
	// 准备输入缓冲区
	wave_in_buffer_hdr_.resize(wave_in_buffer_num_);
	for (std::vector<WAVEHDR>::size_type i = 0; i != wave_in_buffer_hdr_.size(); i++) {
		wave_in_buffer_hdr_[i].lpData = reinterpret_cast<LPSTR>(new Type [buffer_size_points_ * channel_]);
		wave_in_buffer_hdr_[i].dwBufferLength = buffer_size_bytes_;
		wave_in_buffer_hdr_[i].dwBytesRecorded = 0;
		wave_in_buffer_hdr_[i].dwUser = NULL;
		wave_in_buffer_hdr_[i].dwFlags = 0;
		wave_in_buffer_hdr_[i].dwLoops = 1;
		wave_in_buffer_hdr_[i].lpNext = NULL;
		wave_in_buffer_hdr_[i].reserved = 0;
	}
	// 打开输出设备，打开状态保存在 device_state_.wave_out_open_ok ( true | false )
	device_state_.wave_out_open_result = waveOutOpen(&h_wave_out_, WAVE_MAPPER, &wavformat_, NULL, 0, CALLBACK_NULL);
	if (device_state_.wave_out_open_result == MMSYSERR_NOERROR) {
		device_state_.wave_out_open_ok = true;
	}
	// 打开输入设备，打开状态保存在 device_state_.wave_in_open_ok ( true | false )
	// 将输入缓冲区加入输入缓冲队列
	device_state_.wave_in_open_result = waveInOpen(&h_wave_in_, WAVE_MAPPER, &wavformat_, reinterpret_cast<DWORD_PTR>(WaveInProc), reinterpret_cast<DWORD_PTR>(this), CALLBACK_FUNCTION);
	if (device_state_.wave_in_open_result == MMSYSERR_NOERROR) {
		device_state_.wave_in_open_ok = true;
		for (std::vector<WAVEHDR>::size_type i = 0; i != wave_in_buffer_hdr_.size(); i++) {
			waveInPrepareHeader(h_wave_in_, &wave_in_buffer_hdr_[i], sizeof(WAVEHDR));
			waveInAddBuffer(h_wave_in_, &wave_in_buffer_hdr_[i], sizeof(WAVEHDR));
		}
	}
}

template <class Type>
void WinPlay<Type>::Start()
{
	if (state_ == STOP) {
		state_ = PLAYING;
		waveInStart(h_wave_in_);
	}
}

template <class Type>
void WinPlay<Type>::Stop()
{
	if (state_ == PLAYING) {
		state_ = STOP;
		waveInStop(h_wave_in_);
	}
}

// 设置用户定义的音频处理函数
//     处理函数原型 void (*process_func_type)(Type *in, Type *out, uint32 nbytes, void *p_param)
//     in        指向 nbytes 字节的录音数据
//     out       指向播放缓冲区
//     p_param   用户实例数据
template <class Type>
void WinPlay<Type>::SetProcessFunc(process_func_type new_process_func, void *new_p_param)
{
	process_func_ = new_process_func;
	p_param_ = new_p_param;
}

// 音频输入设备回调函数
template <class Type>
void CALLBACK WinPlay<Type>::WaveInProc(HWAVEIN hwi,        
											   UINT uMsg,           
							                   DWORD_PTR dwInstance,    
							                   DWORD_PTR dwParam1,    
						                       DWORD_PTR dwParam2)
{
	LPWAVEHDR pwh = (LPWAVEHDR)dwParam1;  

	WinPlay<Type> *this_obj = reinterpret_cast<WinPlay<Type> *>(dwInstance);

	LPHWAVEOUT phWaveOut = &(this_obj->h_wave_out_);
	WAVEHDR &wavhdr = this_obj->wave_out_wavhdr_;
	std::vector<Buffer> &output_buffer = this_obj->wave_out_buffer_;
	uint32 &r = this_obj->wave_out_buffer_reader_;
	uint32 &w = this_obj->wave_out_buffer_writer_;
	uint32 output_buffer_size = this_obj->wave_out_buffer_num_;
	process_func_type &ProcessFunc = this_obj->process_func_;

	if (WIM_DATA == uMsg && PLAYING == this_obj->state_)  
	{
		// 这里实现了 PING-PONG BUFFER
		// PING-PONG BUFFER 处理流程
		//   1.播放 output_buffer[r] 中的数据，r++，r %= output_buffer_size
		//   2.处理此时得到的录音数据，将处理完的数据写入 output_buffer[w] ，w++，w %= output_buffer_size
		//   3.准备新的录音缓冲区，加入到录音缓冲队列中

		// 1.准备输出缓冲区   
		wavhdr.lpData = (LPSTR)output_buffer[r].ptr;
		wavhdr.dwBufferLength = output_buffer[r].n;
		r++;
		r %= output_buffer_size;
		wavhdr.dwFlags = 0;  
		wavhdr.dwLoops = 0;  
        //
		waveOutPrepareHeader(*phWaveOut, &wavhdr, sizeof(WAVEHDR));  
		waveOutWrite(*phWaveOut, &wavhdr, sizeof(WAVEHDR));  

		// 2.处理录音数据，将处理完的数据写入 output_buffer[w]，ProcessFunc() 是用户定义的处理函数
		if (ProcessFunc) {
			ProcessFunc(reinterpret_cast<Type *>(pwh->lpData), reinterpret_cast<Type *>(output_buffer[w].ptr), pwh->dwBytesRecorded, this_obj->p_param_);
		}
		output_buffer[w].n = pwh->dwBytesRecorded;
		w++;
		w %= output_buffer_size;

		// 3.准备录音缓冲，加入到录音缓冲队列中
		waveInUnprepareHeader(hwi, pwh, sizeof(WAVEHDR));
		waveInPrepareHeader(hwi, pwh, sizeof(WAVEHDR)); 
		waveInAddBuffer(hwi, pwh, sizeof(WAVEHDR));  
	}  
}

#endif // _WIN_PLAY_INL_H
