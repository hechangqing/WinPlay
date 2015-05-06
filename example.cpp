#include <iostream>
#include "WinPlay.h"

#pragma comment(lib, "winmm.lib")

using std::cout;
using std::endl;

template <class T>
void process(T *in, T *out, unsigned int nbytes, void * p_param)
{
	double alpha = *static_cast<double *>(p_param);
	size_t npoints = nbytes / sizeof(T);

	for (size_t i = 0; i != npoints; i++) {
		out[i] = static_cast<T>(in[i] * alpha);
	}
}

int main()
{
	typedef __int16			int16;
	typedef unsigned __int8 uint8;

	WinPlay<int16> audio;
	double alpha = 0.8;
	audio.SetProcessFunc(process<int16>, &alpha);
	if (audio.OK()) {
		audio.Start();
		Sleep(6000);
		audio.Stop();
	} else {
		cout << "设备未正常打开！" << endl;
	}
	return 0;
}
