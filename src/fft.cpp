// Fast Fourier Transformation
// Part of Live Simulator: 2 Extensions
// See copyright notice in LS2X main.cpp

#ifdef LS2X_USE_KISSFFT

#include "fft.h"

namespace ls2x
{
namespace fft
{

kiss_fft_cfg getCfg(size_t sampleSize)
{
	static kiss_fft_cfg cfg = nullptr;
	static size_t prevSmpSize = 0;

	if (!cfg || prevSmpSize != sampleSize)
	{
		kiss_fft_free(cfg);
		cfg = kiss_fft_alloc(int(prevSmpSize = sampleSize), false, nullptr, nullptr);
	}

	return cfg;
}

void fftr(const short *input, kiss_fft_scalar *out_l, kiss_fft_scalar *out_r, size_t sampleSize)
{
	kiss_fft_cfg kcfg = getCfg(sampleSize);
	// input: left at 0, right at 1
	// output: left is 2, right is 3
	kiss_fft_cpx *ptrdata = new kiss_fft_cpx[sampleSize * 4];
	memset(ptrdata + sampleSize * 2, 0, sampleSize * 2 * sizeof(kiss_fft_cpx));

	for (size_t i = 0; i < sampleSize; i++)
	{
		ptrdata[i * 2 + 0] = {kiss_fft_scalar(input[i * 2]) / kiss_fft_scalar(32767), 0};
		ptrdata[i * 2 + 1] = {kiss_fft_scalar(input[i * 2 + 1]) / kiss_fft_scalar(32767), 0};
	}

	kiss_fft(kcfg, ptrdata, ptrdata + sampleSize * 2);
	kiss_fft(kcfg, ptrdata + sampleSize, ptrdata + sampleSize * 3);

	// I think this one is very good when parallelized
	for (size_t i = 0; i < sampleSize; i++)
	{
		kiss_fft_cpx &l = (ptrdata + sampleSize * 2)[i];
		kiss_fft_cpx &r = (ptrdata + sampleSize * 3)[i];
		out_l[i] = abs(l.i * l.i + l.r + l.r);
		out_r[i] = abs(r.i * r.i + r.r * r.r);
	}

	delete[] ptrdata;
}
// stereo input > mono merged fftr (or mono input > mono fftr)
void fftr(const short *input, kiss_fft_scalar *out, size_t sampleSize, bool stereo)
{
	kiss_fft_cfg kcfg = getCfg(sampleSize);

	if (stereo)
	{
		// stereo input, avg. fft each channel
		kiss_fft_cpx *ptrdata = new kiss_fft_cpx[sampleSize * 4];
		memset(ptrdata + sampleSize * 2, 0, sampleSize * 2 * sizeof(kiss_fft_cpx));

		for (size_t i = 0; i < sampleSize; i++)
		{
			ptrdata[i * 2 + 0] = {kiss_fft_scalar(input[i * 2]) / kiss_fft_scalar(32767), 0};
			ptrdata[i * 2 + 1] = {kiss_fft_scalar(input[i * 2 + 1]) / kiss_fft_scalar(32767), 0};
		}

		kiss_fft(kcfg, ptrdata, ptrdata + sampleSize * 2);
		kiss_fft(kcfg, ptrdata + sampleSize, ptrdata + sampleSize * 3);

		// I think this one is very good when parallelized
		for (size_t i = 0; i < sampleSize; i++)
		{
			kiss_fft_cpx &l = (ptrdata + sampleSize * 2)[i];
			kiss_fft_cpx &r = (ptrdata + sampleSize * 3)[i];
			out[i] = (abs(l.i * l.i + l.r + l.r) + abs(r.i * r.i + r.r * r.r)) * kiss_fft_scalar(0.5);
		}

		delete[] ptrdata;
	}
	else
	{
		// mono input, mono putput
		kiss_fft_cpx *ptrdata = new kiss_fft_cpx[sampleSize * 2];
		memset(ptrdata + sampleSize, 0, sampleSize * sizeof(kiss_fft_cpx));

		for (size_t i = 0; i < sampleSize; i++)
		{
			ptrdata[i] = {kiss_fft_scalar(input[i]) / kiss_fft_scalar(32767), 0};
		}

		kiss_fft(kcfg, ptrdata, ptrdata + sampleSize);

		for (size_t i = 0; i < sampleSize; i++)
		{
			kiss_fft_cpx &l = (ptrdata + sampleSize)[i];
			out[i] = abs(l.i * l.i + l.r + l.r);
		}

		delete[] ptrdata;
	}
}

// very ugly
const char *scalarType()
{
#define ASDAWDAWDWA(x) #x
#define ADJWADUAWDA(x) ASDAWDAWDWA(x)
	return ADJWADUAWDA(kiss_fft_scalar);
#undef ADJWADUAWDA
#undef ASDAWDAWDWA
}

const std::map<std::string, void*> &getFunctions()
{
	static std::map<std::string, void*> funcs = {
		{"fftr1", (void*) (void(*)(const short *, kiss_fft_scalar *, kiss_fft_scalar *, size_t))fftr},
		{"fftr2", (void*) (void(*)(const short *, kiss_fft_scalar *, size_t, bool))fftr},
		{"scalarType", (void*) scalarType}
	};
	return funcs;
}

}
}
#endif
