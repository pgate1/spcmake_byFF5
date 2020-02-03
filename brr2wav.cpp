
#include <stdio.h>
#include <sys/stat.h>

#pragma warning( disable : 4786 )
#include<string>
#include<vector>
#include<iterator>
using namespace std;

typedef unsigned char uint8;
typedef signed char int8;
typedef unsigned short uint16;
typedef signed short int16;
typedef unsigned long uint32;
typedef signed long int32;

static inline int32 sclip15(int32 x) {
	return ((x & 16384) ? (x | ~16383) : (x & 16383));
}

static inline int32 sclamp16(int32 x) {
	return ((x > 32767) ? 32767 : (x < -32768) ? -32768 : x);
}

vector<int16> decode_brr(const uint8 *brr, int size, bool *ptr_looped)
{
	vector<int16> raw_samples;

	int prev[2] = { 0, 0 };
	int decoded_size = 0;
	while(decoded_size + 9 <= size){
		const uint8 *brr_chunk = &brr[decoded_size];

		uint8 flags = brr_chunk[0];
		bool chunk_end = (flags & 1) != 0;
		bool chunk_loop = (flags & 2) != 0;
		uint8 filter = (flags >> 2) & 3;
		uint8 range = flags >> 4;
		bool valid_range = (range <= 0x0C);

		int S1 = prev[0];
		int S2 = prev[1];

		int byte_index;
		for(byte_index=0; byte_index<8; byte_index++){
			int8 sample1 = brr_chunk[1 + byte_index];
			int8 sample2 = sample1 << 4;
			sample1 >>= 4;
			sample2 >>= 4;

			int nibble;
			for(nibble=0; nibble<2; nibble++){
				int32 out;
				out = nibble ? (int)sample2 : (int)sample1;
				out = valid_range ? ((out << range) >> 1) : (out & ~0x7FF);

				switch (filter){
				case 0: // Direct
					break;
				case 1: // 15/16
					out += S1 + ((-S1) >> 4);
					break;
				case 2: // 61/32 - 15/16
					out += (S1 << 1) + ((-((S1 << 1) + S1)) >> 5) - S2 + (S2 >> 4);
					break;
				case 3: // 115/64 - 13/16
					out += (S1 << 1) + ((-(S1 + (S1 << 2) + (S1 << 3))) >> 6) - S2 + (((S2 << 1) + S2) >> 4);
					break;
				}

				out = sclip15(sclamp16(out));

				S2 = S1;
				S1 = out;

				raw_samples.push_back(out << 1);
			}
		}

		prev[0] = S1;
		prev[1] = S2;

		decoded_size += 9;

		if(chunk_end){
			if(ptr_looped != NULL){
				*ptr_looped = chunk_loop;
			}
			break;
		}
	}

	return raw_samples;
}

static void write16(vector<uint8> &data, uint16 value)
{
	data.push_back(value & 0xff);
	data.push_back((value >> 8) & 0xff);
}

static void write32(vector<uint8> &data, uint32 value)
{
	data.push_back(value & 0xff);
	data.push_back((value >> 8) & 0xff);
	data.push_back((value >> 16) & 0xff);
	data.push_back((value >> 24) & 0xff);
}

static void write(vector<uint8> &data, const char *value, int size)
{
	for(int i=0; i<size; i++){
		data.push_back(value[i]);
	}
}

class WavWriter
{
public:
	int16 channels;
	int32 samplerate;
	int16 bitwidth;

	string m_message;

	vector<int16> samples;
	int32 loop_sample;
	bool looped;

public:
/*
	WavWriter()
	{
		channels = 1;	
		samplerate = 44100;
		bitwidth = 16;
		loop_sample = 0;
		looped = false;
	}
	*/
	WavWriter(vector<int16> samples)
	{
		channels = 1;
		samplerate = 44100;
		bitwidth = 16;
		loop_sample = 0;
		looped = false;
		this->samples.assign(samples.begin(), samples.end());
	}

	inline void SetLoopSample(int32 loop_sample) {
		this->loop_sample = loop_sample;
		this->looped = true;
	}

	void AddSample(int16 sample);
	void AddSample(vector<int16> samples);
	bool WriteFile(const string &filename);
};


void WavWriter::AddSample(int16 sample)
{
	samples.push_back(sample);
}

void WavWriter::AddSample(vector<int16> samples)
{
	std::copy(samples.begin(), samples.end(), std::back_inserter(this->samples));
}

bool WavWriter::WriteFile(const string &filename)
{
	int16 bytes_per_sample = bitwidth / 8;
	if (bitwidth != 16) {
		printf("Unsupported bitwidth");
		return false;
	}

	FILE *wav_file = fopen(filename.c_str(), "wb");
	if (wav_file == NULL) {
		printf("File open error");
		return false;
	}

	std::vector<uint8> header;
	header.reserve(36);
	write(header, "RIFF", 4);
	write32(header, 0); // 0x04: put actual file size later
	write(header, "WAVE", 4);
	write(header, "fmt ", 4);
	write32(header, 16);
	write16(header, 1);
	write16(header, channels);
	write32(header, samplerate);
	write32(header, samplerate * bytes_per_sample * channels);
	write16(header, bytes_per_sample * channels);
	write16(header, bitwidth);

	std::vector<uint8> data_chunk;
	data_chunk.reserve(8 + samples.size() * 2);
	write(data_chunk, "data", 4);
	write32(data_chunk, (uint32)(samples.size() * 2));
	vector<int16>::iterator itr_sample;
	for(itr_sample=samples.begin(); itr_sample!=samples.end(); ++itr_sample){
		int16 sample = *itr_sample;
		write16(data_chunk, sample);
	}

	std::vector<uint8> smpl_chunk;
	// add loop point info (smpl chunk) if needed... details:
	// en: http://www.blitter.com/~russtopia/MIDI/~jglatt/tech/wave.htm
	// ja: http://co-coa.sakura.ne.jp/index.php?Sound%20Programming%2FWave%20File%20Format
	if (looped) {
		smpl_chunk.reserve(8 + 60);
		write(smpl_chunk, "smpl", 4);
		write32(smpl_chunk, 60); // chunk size
		write32(smpl_chunk, 0);  // manufacturer
		write32(smpl_chunk, 0);  // product
		write32(smpl_chunk, 1000000000 / samplerate); // sample period
		write32(smpl_chunk, 60); // MIDI uniti note (C5)
		write32(smpl_chunk, 0);  // MIDI pitch fraction
		write32(smpl_chunk, 0);  // SMPTE format
		write32(smpl_chunk, 0);  // SMPTE offset
		write32(smpl_chunk, 1);  // sample loops
		write32(smpl_chunk, 0);  // sampler data
		write32(smpl_chunk, 0);  // cue point ID
		write32(smpl_chunk, 0);  // type (loop forward)
		write32(smpl_chunk, loop_sample); // start sample #
		write32(smpl_chunk, (uint32)samples.size() / channels); // end sample #
		write32(smpl_chunk, 0);  // fraction
		write32(smpl_chunk, 0);  // playcount
	}

	uint32 whole_size = (uint32)(header.size() + data_chunk.size() + smpl_chunk.size() - 8);
	header[4] = whole_size & 0xff;
	header[5] = (whole_size >> 8) & 0xff;
	header[6] = (whole_size >> 16) & 0xff;
	header[7] = (whole_size >> 24) & 0xff;

	if (fwrite(&header[0], header.size(), 1, wav_file) != 1) {
		printf("File write error");
		fclose(wav_file);
		return false;
	}

	if (fwrite(&data_chunk[0], data_chunk.size(), 1, wav_file) != 1) {
		printf("File write error");
		fclose(wav_file);
		return false;
	}

	if (smpl_chunk.size() != 0) {
		if (fwrite(&smpl_chunk[0], smpl_chunk.size(), 1, wav_file) != 1) {
			printf("File write error");
			fclose(wav_file);
			return false;
		}
	}

	fclose(wav_file);
	return true;
}
/*
int brr2wav(const string &brr_filename, uint16 pitch)
{
	struct stat statBuf;
	int brr_size = 0;
	if(stat(brr_filename.c_str(), &statBuf)==0) brr_size = statBuf.st_size;
	if(brr_size==0) return -1;

	FILE *brrfp = fopen(brr_filename.c_str(), "rb");
	if(brrfp==NULL){
		printf("brr file dont find\n");
		return -1;
	}

	uint8 *brr_data = new uint8[brr_size];
	fread(brr_data, 1, brr_size, brrfp);
	fclose(brrfp);
   
	uint16 loop_offset = brr_data[0] | (brr_data[1] << 8);

*/
int brr2wav(const string &wav_filename, const uint8 *brr_data, int brr_size, uint16 loop_offset, uint16 pitch)
{
	int loop_sample = loop_offset / 9 * 16;

//	string wav_filename = brr_filename.substr(0, brr_filename.length()-3) + "wav";

	bool looped = false;
	WavWriter wave(decode_brr(brr_data, brr_size, &looped));
	wave.samplerate = pitch * 32000 / 0x1000;
	wave.bitwidth = 16;
	wave.channels = 1;
	if(looped){
		wave.SetLoopSample(loop_sample);
	}

	if(!wave.WriteFile(wav_filename)) {
	//	delete[] brr_data;
		return false;
	}

	//delete[] brr_data;
	return true;
}
/*
int main(int argc, char *argv[])
{
	uint16 pitch = 0x1000;

	int argi;
	for(argi=1; argi<argc; argi++){
		string brr_filename(argv[argi]);
		brr2wav(brr_filename, pitch);
	}

	return 0;
}
*/
