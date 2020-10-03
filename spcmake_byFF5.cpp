/*
	spcmake_byFF5
	Copyright (c) 2019 pgate1
*/

#include<stdio.h>
#include<memory.h>
#include<sys/stat.h>

#pragma warning( disable : 4786 )
#include<string>
#include<map>
#include<vector>
#include<set>
using namespace std;

typedef unsigned char  uint8;
typedef unsigned short uint16;
typedef unsigned long  uint32;

static const int FF5_BRR_NUM = 35;

class FF5_AkaoSoundDriver
{
public:
	uint32 driver_size;
	uint8 *driver;

	// ���ʉ��V�[�P���X
	uint32 eseq_size;
	uint8 *eseq;

	// �풓�g�`
	uint32 sbrr_size;
	uint8 *sbrr;
	uint16 sbrr_start[16]; // �X�^�[�g�A�h���X�ƃ��[�v�A�h���X
	uint16 sbrr_tune[8];
	uint16 sbrr_adsr[8];

	// �g�`
	uint32 brr_size[FF5_BRR_NUM];
	uint8 *brr[FF5_BRR_NUM];
	uint16 brr_loop[FF5_BRR_NUM];
	uint16 brr_tune[FF5_BRR_NUM];
	uint16 brr_adsr[FF5_BRR_NUM];

	FF5_AkaoSoundDriver()
	{
		driver = NULL;
		eseq = NULL;
		sbrr = NULL;
		int i;
		for(i=0; i<FF5_BRR_NUM; i++){
			brr[i] = NULL;
		}
	}

	~FF5_AkaoSoundDriver()
	{
		if(driver!=NULL) delete[] driver;
		if(eseq!=NULL) delete[] eseq;
		if(sbrr!=NULL) delete[] sbrr;
		int i;
		for(i=0; i<FF5_BRR_NUM; i++){
			if(brr[i]!=NULL) delete[] brr[i];
		}
	}

	int get_akao(const char *rom_fname);
};

// FinalFantasy5.rom����Akao�T�E���h�h���C�o���擾
int FF5_AkaoSoundDriver::get_akao(const char *rom_fname)
{
	FILE *fp = fopen(rom_fname, "rb");
	if(fp==NULL){
		printf("cant open %s.\n", rom_fname);
		return -1;
	}
	struct stat statBuf;
	int rom_size = 0;
	if(stat(rom_fname, &statBuf)==0) rom_size = statBuf.st_size;
	if(rom_size!=2*1024*1024){
		printf("FF5��rom�T�C�Y���Ⴄ�H�w�b�_���t���Ă�H\n");
		return -1;
	}
	uint8 *rom = new uint8[2*1024*1024];
	fread(rom, 1, 2*1024*1024, fp);
	fclose(fp);

	// FF5��ROM���m�F
	if(rom[0]!=0x78){
		delete[] rom;
		printf("FF5��rom�Ƀw�b�_���t���Ă���悤�ł��H\n");
		return -1;
	}
	if(!(rom[0x0FFC0]=='F' && rom[0x0FFC6]=='F' && rom[0x0FFCE]=='5')){
		delete[] rom;
		printf("FF5��rom�ł͂Ȃ��H\n");
		return -1;
	}

/*
�����g�p��32�܂�

0x0000 - 0x01FF�@���[�N������
0x0200 - 0x19EF�@�T�E���h�h���C�o
0x1A00 - 0x1A0F�@�풓�g�`�����␳
0x1A40 - 0x1A7F�@���������␳
0x1A80 - 0x1A8F�@�풓�g�`ADSR
0x1AC0 - 0x1AFF�@����ADSR
0x1B00 - 0x1B1F�@�풓�g�`BRR�A�h���X
0x1B80 - 0x1BFF�@����BRR�A�h���X
0x1C00 - 0xXXXX�@�V�[�P���X
0x2C00 - 0x47FF�@���ʉ��V�[�P���X�ƌ��ʉ�BRR�i�����Ă������j
0x4800 - 0x490D�@�풓�g�`BRR�i�ړ�ok�j
0x490E - 0xXXXX�@����BRR�i�ړ�ok�j
0xD200 - 0xF9FF�@�G�R�[�o�b�t�@�i�ړ�ok�j
0xFA00 - 0xFFFF�@���[�N������
*/

	// �T�E���h�h���C�o
	// 0x04064D 2�o�C�g�̓T�C�Y
	// 0x04064F - 0x041E3E -> 0x0200 - 0x19EF
	driver_size = *(uint16*)(rom+0x4064D);
	driver = new uint8[driver_size];
	memcpy(driver, rom+0x4064F, driver_size);
//FILE *fp=fopen("out.bin","wb");fwrite(audio.driver,1,audio.driver_size,fp);fclose(fp);


	// �풓�g�`BRR
	// 0x041E3F 2�o�C�g�̓T�C�Y(0x010E)
	// 0x041E41 - 0x041F4E -> 0x4800 - 0x490D
	sbrr_size = *(uint16*)(rom+0x41E3F);
	sbrr = new uint8[sbrr_size];
	memcpy(sbrr, rom+0x41E41, sbrr_size);
//{FILE *fp=fopen("out.bin","wb");fwrite(asd.sbrr,1,asd.sbrr_size,fp);fclose(fp);}

	// �풓�g�`BRR�A�h���X
	// 0x041F4F 2�o�C�g�̓T�C�Y
	// 0x041F51 - 0x041F70 -> 0x1B00 - 0x1B1F
	// 0x4800-0x48FC���w��
	memcpy(sbrr_start, rom+0x41F51, 32); // (2+2)byte x 8

	// �풓�g�`ADSR
	// 0x041F71 2�o�C�g�̓T�C�Y
	// 0x041F73 - 0x041F82 -> 0x1A80 - 0x1A8F
	memcpy(sbrr_adsr, rom+0x41F73, 16); // 2byte x 8

	// �풓�g�`�����␳
	// 0x041F83 2�o�C�g�̓T�C�Y
	// 0x041F85 - 0x041F94 -> 0x1A00 - 0x1A0F
	memcpy(sbrr_tune, rom+0x41F85, 16); // 2byte x 8


	// ����BRR
	// 0x043C6F - 0x043CD7 -> 0x490E �` �K�v�ȏꏊ�܂�
	int i;
	for(i=0; i<FF5_BRR_NUM; i++){
		// �擪2�o�C�g�̓T�C�Y
		int brr_adrs = *(uint32*)(rom+0x43C6F+i*3) & 0x001FFFFF;
		brr_size[i] = *(uint16*)(rom+brr_adrs);
		brr[i] = new uint8[brr_size[i]];
		memcpy(brr[i], rom+brr_adrs+2, brr_size[i]);
	}
	// �������[�v
	// 0x043CD8 - 0x043D1D
	memcpy(brr_loop, rom+0x43CD8, 2*FF5_BRR_NUM); // 2byte x 35
/*
system("mkdir brr");
char fname[100];
for(i=0; i<FF5_BRR_NUM; i++){
	sprintf(fname, "brr/ff5_%02X.brr", i);
	FILE *fp = fopen(fname, "wb");
	fwrite(&brr_loop[i], 1, 2, fp);
	fwrite(brr[i], 1, brr_size[i], fp);
	fclose(fp);
}
*/
	// ���������␳
	// 0x043D1E - 0x043D63 -> 0x1A40 - 0x1A7F
	memcpy(brr_tune, rom+0x43D1E, 2*FF5_BRR_NUM); // 2byte x 35

	// ����ADSR
	// 0x043D64 - 0x043DA9 -> 0x1AC0 - 0x1AFF
	memcpy(brr_adsr, rom+0x43D64, 2*FF5_BRR_NUM); // 2byte x 35


	// ���ʉ��V�[�P���X�ƌ��ʉ�BRR
	// 0x041F95 2�o�C�g�̓T�C�Y
	// 0x041F97 - 0x043B96 -> 0x2C00 - 0x47FF
	// 0x2C00�`���ʉ��V�[�P���X�A�h���X
	// 0x3000�`���ʉ��V�[�P���X
	eseq_size = *(uint16*)(rom+0x41F95);
	eseq = new uint8[eseq_size];
	memcpy(eseq, rom+0x41F97, eseq_size);
	eseq[339*2] = 0xBB; // ���ʉ�339�͌��H
//uint8 buf[0x2C00];memset(buf,0x00,0x2C00);
//FILE *fp=fopen("out.bin","wb");fwrite(buf,1,0x2C00,fp);fwrite(effect_seq,1,size,fp);fclose(fp);
/*
// ���ʉ��V�[�P���X�o�̓e�X�g
{
system("mkdir effect");
FILE *efp = fopen("effect/effall.txt", "w");
int pass = 0;
int i;
for(i=0; i<8*44+2; i++){ // 354
	uint16 adrs = *(uint16*)(asd.eseq+i*2);
	if(adrs==0x0000){
		pass++;
		continue;
	}
//	if(adrs==0x46BA) adrs++;
	printf("%d 0x%04X\n", i, adrs);
	fprintf(efp, "@@%d ", i);
	char fname[30];
	sprintf(fname, "effect/ff5e_%03d.txt", i);
	FILE *fp = fopen(fname, "w");
	adrs -= 0x2C00;
	int j;
	for(j=adrs; ; j++){
		fprintf(fp, "%02X ", asd.eseq[j]);
	//	if(asd.eseq[j]==0xF9){
	//		printf("F9\n"); getchar();
	//	}
		if(asd.eseq[j]==0xF2) break;
	}
	j++;
	fclose(fp);
	uint16 next_adrs = *(uint16*)(asd.eseq+(i+1)*2);
	if(next_adrs!=0x0000 && (j+0x2C00)<next_adrs){
		printf("err %d 0x%04X 0x%04X\n", i, j+0x2C00, next_adrs);
		getchar();
	}
}
fclose(efp);
printf("pass %d\n", pass);getchar();
}
*/

	delete[] rom;

	return 0;
}

struct FF5_TONE {
	string brr_fname;
	bool used; // formatter only
	int line; // formatter only
	int brr_id; // formatter only
	int inst_id; // �풓�g�` only
	uint8 octave; // formatter only
	uint8 transpose; // formatter only
	uint8 detune; // formatter only
	uint8 adsr1, adsr2;
};

class FF5_SPC
{
public:
	string songname;
	string gametitle;
	string artist;
	string dumper;
	string comment;

	uint32 play_time;
	uint32 fade_time;

	map<int, FF5_TONE> brr_map;

	uint8 *seq[8];
	uint32 seq_size[8];

	uint16 track_loop[8]; // 0xFA 0xFFFF�̏ꍇ�̓g���b�N���[�v�Ȃ�
	vector<uint16> break_point[8]; // 0xF9
	vector<uint16> jump_point[8]; // 0xFA

	uint16 brr_offset;
	bool f_brr_echo_overcheck;
	uint16 echo_depth;
	bool f_surround; // �t�ʑ��T���E���h
	bool f_eseq_out; // ���ʉ��V�[�P���X���ߍ���

	FF5_SPC()
	{
		play_time = 300; // �b�A�f�t�H���g�Đ�����
		fade_time = 10000; // �~���b�A�f�t�H���g�t�F�[�h�A�E�g����
		int i;
		for(i=0; i<8; i++){
			seq[i] = NULL;
			seq_size[i] = 0;
			track_loop[i] = 0xFFFF;
		}
		// 0x2C00 or 0x3000 or 0x4800����BRR, �ŏ���0x10E(0x10D�܂�)�͏풓�g�`BRR
		brr_offset = 0x4800;
		f_brr_echo_overcheck = false;
		echo_depth = 5;
		f_surround = false;
		f_eseq_out = true;
	}

	~FF5_SPC()
	{
		int i;
		for(i=0; i<8; i++){
			if(seq[i]!=NULL){
				delete[] seq[i];
				seq[i] = NULL;
			}
		}
	}
};

class spcmake_byFF5
{
public:
	FF5_AkaoSoundDriver asd;
	FF5_SPC spc;
	string str;

	int read_mml(const char *mml_fname);
	int formatter(void);
	int get_sequence(void);
	int get_ticks(int track);
	int make_spc(const char *spc_fname);
};

int spcmake_byFF5::read_mml(const char *mml_fname)
{
	// �V�[�P���X�t�@�C���ǂݍ���
	FILE *fp = fopen(mml_fname, "r");
	if(fp==NULL){
		printf("HexMML�t�@�C�� %s ���J���܂���.\n", mml_fname);
		return -1;
	}
	char buf[1024];
	while(fgets(buf, 1023, fp)){
		str += buf;
	}
	fclose(fp);
	return 0;
}

int line;

int skip_space(const string &str, int p)
{
	while(str[p]==' ' || str[p]=='\t' || str[p]=='\r' || str[p]=='\n'){
		if(str[p]=='\n') line++;
		p++;
	}
	return p;
}

int term_end(const string &str, int p)
{
	while(str[p]!=' ' && str[p]!='\t' && str[p]!='\r' && str[p]!='\n' && str[p]!='\0') p++;
	return p;
}

int term_begin(const string &str, int p)
{
	while(str[p]!=' ' && str[p]!='\t' && str[p]!='\r' && str[p]!='\n' && str[p]!='\0') p--;
	return p;
}

int num_end(const string &str, int p)
{
	while(isdigit(str[p])) p++;
	return p;
}

int get_hex(const string &str, int p)
{
	char buf[3];
	buf[0] = str[p];

	char c = str[p+1];
	if(!((c>='0' && c<='9') || (c>='A' && c<='F'))){
		printf("Error line %d : 16�i���\�L�ُ�ł�.\n", line);
		getchar();
	}

	buf[1] = str[p+1];
	buf[2] = '\0';
	int hex;
	sscanf(buf, "%02x", &hex);
	return hex;
}

int is_space(const char c)
{
// isspace() 0x20 0x09 0x0D 0x0A     0x0B 0x0C
	if(c==' ' || c=='\t' || c=='\r' || c=='\n' || c=='\0'){
		return 1;
	}
	return 0;
}

int is_cmd(const char c)
{
	if((c>='0' && c<='9') || (c>='A' && c<='F')){
		return 1;
	}
	return 0;
}

int spcmake_byFF5::formatter(void)
{
	line = 1;

	// �R�����g�폜
	int sp = 0;
	for(;;){
		sp = str.find("/*", sp);
		if(sp==string::npos) break;
		int ep = str.find("*/", sp+2);
		if(ep==string::npos) break;
		int k = 0;
		int p;
		for(p=sp; p<ep; p++) if(str[p]=='\n') k++;
		str.erase(sp, ep-sp+2);
		if(k) str.insert(sp, k, '\n');
	}
	sp = 0;
	for(;;){
		sp = str.find("//", sp);
		if(sp==string::npos) break;
		int ep = str.find('\n', sp+2);
		if(ep==string::npos) break;
		str.erase(sp, ep-sp);
	}
	
//{FILE *fp=fopen("sample_debug.txt","w");fprintf(fp,str.c_str());fclose(fp);}

	#define _MML_END_ "#track ;"
	// �Ō�� _MML_END_ ��t��
	str.insert(str.length(), "\n"_MML_END_" ");
//{FILE *fp=fopen("sample_debug.txt","w");fprintf(fp,str.c_str());fclose(fp);}return -1;

	char buf[1024];
	int track_num = 0;
	map<string, FF5_TONE> tone_map;
	int brr_id = 0;
	bool f_octave_swap = false;
	bool f_auto_assign_toneid = false;
	set<string> label_set;
	map<string, int> label_map;

	int p;
	for(p=0; str[p]!='\0'; p++){
		if(str[p]=='\n'){
			line++;
		//	printf("line %d %s\n", line, str.c_str()+p+1);getchar();
		}

		if(str[p]=='#'){

			// �Ȗ��̎擾
			if(str.substr(p, 9)=="#songname"){
				int sp = str.find('"', p+9) + 1;
				int ep = str.find('"', sp);
				spc.songname = str.substr(sp, ep-sp);
				if(spc.songname.length()>32){
					printf("�x�� line %d : #songname��32�o�C�g�𒴂��Ă��܂�.\n", line);
					getchar();
				}
				str.erase(p, ep-p+1);
				p--;
				continue;
			}

			// �Q�[�����̎擾
			if(str.substr(p, 10)=="#gametitle"){
				int sp = str.find('"', p+10) + 1;
				int ep = str.find('"', sp);
				spc.gametitle = str.substr(sp, ep-sp);
				if(spc.gametitle.length()>32){
					printf("�x�� line %d : #gametitle��32�o�C�g�𒴂��Ă��܂�.\n", line);
					getchar();
				}
				str.erase(p, ep-p+1);
				p--;
				continue;
			}

			// ��Ȏ҂̎擾
			if(str.substr(p, 7)=="#artist"){
				int sp = str.find('"', p+7) + 1;
				int ep = str.find('"', sp);
				spc.artist = str.substr(sp, ep-sp);
				if(spc.artist.length()>32){
					printf("�x�� line %d : #artist��32�o�C�g�𒴂��Ă��܂�.\n", line);
					getchar();
				}
				str.erase(p, ep-p+1);
				p--;
				continue;
			}

			// �쐬�҂̎擾
			if(str.substr(p, 7)=="#dumper"){
				int sp = str.find('"', p+7) + 1;
				int ep = str.find('"', sp);
				spc.dumper = str.substr(sp, ep-sp);
				if(spc.dumper.length()>16){
					printf("�x�� line %d : #dumper��16�o�C�g�𒴂��Ă��܂�.\n", line);
					getchar();
				}
				str.erase(p, ep-p+1);
				p--;
				continue;
			}

			// �R�����g�̎擾
			if(str.substr(p, 8)=="#comment"){
				int sp = str.find('"', p+8) + 1;
				int ep = str.find('"', sp);
				spc.comment = str.substr(sp, ep-sp);
				if(spc.comment.length()>32){
					printf("�x�� line %d : #comment��32�o�C�g�𒴂��Ă��܂�.\n", line);
					getchar();
				}
				str.erase(p, ep-p+1);
				p--;
				continue;
			}

			// �Đ����Ԃƃt�F�[�h�A�E�g���Ԃ̐ݒ�
			if(str.substr(p, 7)=="#length"){
				// �Đ�����
				int sp = skip_space(str, p+7);
				int ep = term_end(str, sp);
				string sec_str = str.substr(sp, ep-sp);
				int cp;
				if((cp=sec_str.find(':'))!=string::npos){ // ��F2:30
					sec_str[cp] = '\0';
					spc.play_time = atoi(sec_str.c_str()) * 60 + atoi(sec_str.c_str()+cp+1);
				}
				else{ // ��F150
					spc.play_time = atoi(sec_str.c_str());
				}

				// �t�F�[�h�A�E�g����
				sp = skip_space(str, ep);
				ep = term_end(str, sp);
				spc.fade_time = atoi(str.substr(sp, ep-sp).c_str());
				
				str.erase(p, ep-p);
				p--;
				continue;
			}

			// BRR�I�t�Z�b�g
			if(str.substr(p, 11)=="#brr_offset"){

				// �I�t�Z�b�g���͌��ʉ��V�[�P���X�͏o�͂��Ȃ�
				spc.f_eseq_out = false;

				int sp = skip_space(str, p+11);
				int ep = term_end(str, sp);
				if(str.substr(sp, ep-sp)=="auto"){
					spc.brr_offset = 0xFFFF;
				}
				else{ // 0x3000 �Ƃ�
					int brr_offset = strtol(str.substr(sp, ep-sp).c_str(), NULL, 16);
				//	printf("brr_offset 0x%04X\n", brr_offset);
					if(brr_offset<0 || brr_offset>=0x10000){
						printf("Error line %d : #brr_offset�̒l���s���ł�.\n", line);
						return -1;
					}
					spc.brr_offset = brr_offset;
				}
				str.erase(p, ep-p);
				p--;
				continue;
			}

			// BRR�̈悪�G�R�[�o�b�t�@�ɏd�Ȃ�̃`�F�b�N��L���ɂ���
			if(str.substr(p, 19)=="#brr_echo_overcheck"){
				spc.f_brr_echo_overcheck = true;
				str.erase(p, 19);
				p--;
				continue;
			}

			// �G�R�[�[���w��
			if(str.substr(p, 11)=="#echo_depth"){
				int sp = skip_space(str, p+11);
				if(str[sp]=='#'){
					printf("Error : #echo_depth �p�����[�^���w�肵�Ă�������.\n");
					return -1;
				}
				int ep = term_end(str, sp);
				// EDL default 5
				int echo_depth = atoi(str.substr(sp, ep-sp).c_str());
				if(echo_depth<=0 || echo_depth>=16){
					printf("Error line %d : #echo_depth �� 1�`15 �Ƃ��Ă�������.\n", line);
					return -1;
				}
				spc.echo_depth = echo_depth;
				str.erase(p, ep-p);
				p--;
				continue;
			}

			// �t�ʑ��T���E���h�L��
			if(str.substr(p, 9)=="#surround"){
				spc.f_surround = true;
				str.erase(p, 9);
				p--;
				continue;
			}

			// �I�N�^�[�u�R�}���h����ւ�
			if(str.substr(p, 7)=="#swap<>" || str.substr(p, 7)=="#swap><"){
				f_octave_swap ^= 1;
				str.erase(p, 7);
				p--;
				continue;
			}

			// tone_id���ȗ����Ɏ������蓖��
			if(str.substr(p, 19)=="#auto_assign_toneid"){
				f_auto_assign_toneid = true;
				str.erase(p, 19);
				p--;
				continue;
			}

			// �g�`�錾
			if(str.substr(p, 5)=="#tone"){
				int sp = skip_space(str, p+5); // tone�w��擪
				if(str[sp]=='"'){
					// tone_id�ȗ�����Ă���ꍇ�͓����I�Ɋ��蓖�Ă�
					if(f_auto_assign_toneid){
						static int auto_id = 32;
						char id_buf[10];
						sprintf(id_buf, "%d ", auto_id++);
						str.insert(sp, id_buf);
					}
					else{
						printf("Error line %d : #tone �� tone_id ���w�肳��Ă��܂���.\n", line);
						return -1;
					}
				}
				int ep = term_end(str, sp);
				string tone_id = str.substr(sp, ep-sp);
				//printf("tone_id[%s]\n",tone_id.c_str());getchar();
				if(tone_map.find(tone_id)!=tone_map.end()){
					printf("Error line %d : #tone %s �͂��łɐ錾����Ă��܂�.\n", line, tone_id.c_str());
					return -1;
				}
				// �풓�g�`�ł��I�N�^�[�u�E�g�����X�|�[�Y�E�f�B�`���[���ݒ肷�邩��ǉ�
				tone_map[tone_id].used = false; // tone�ǉ�
				tone_map[tone_id].line = line;

				// brr_fname�̎擾
				sp = skip_space(str, ep) + 1;
				ep = term_end(str, sp);
				string brr_fname = str.substr(sp, ep-sp-1);
				// �풓�g�`�Ȃ�true
				bool f_stayinst = brr_fname.substr(0,9)=="FF5inst:s";
				if(brr_fname.substr(0,8)=="FF5inst:"){
					int ssp = f_stayinst ? 9 : 8;
					int eep = term_end(brr_fname, ssp);
					int inst_id = strtol(brr_fname.substr(ssp, eep-ssp).c_str(), NULL, 16);
					if(!f_stayinst && (inst_id<0 || inst_id>=FF5_BRR_NUM)){
						printf("Error line %d : FF5inst �g�`�w��� 00�`22(16�i��) �Ƃ��Ă�������.\n", line);
						return -1;
					}
					if(f_stayinst && (inst_id<0 || inst_id>7)){
						printf("Error line %d : FF5inst �풓�g�`�w��� s0�`s7 �Ƃ��Ă�������.\n", line);
						return -1;
					}
					tone_map[tone_id].inst_id = inst_id;
				}
				else{
					struct stat st;
					if(stat(brr_fname.c_str(), &st)!=0){
						printf("Error line %d : BRR�t�@�C�� %s ������܂���.\n", line, brr_fname.c_str());
						return -1;
					}
					if((st.st_size-2)%9){
						printf("Error line %d : BRR�t�@�C�� %s �T�C�Y�ُ�ł�.���[�v�A�h���X���t������Ă��Ȃ��H\n", line, brr_fname.c_str());
						return -1;
					}
				}
				// �풓�g�`����Ȃ����BRR��ǉ�����
				if(!f_stayinst){
					if(brr_id>=32){
						printf("Error line %d : tone�錾(BRR�w��)��32�܂łł�.\n", line);
						return -1;
					}
					spc.brr_map[brr_id].brr_fname = brr_fname;
				}
				tone_map[tone_id].brr_fname = brr_fname;

				// �p�����[�^�擾�A#tone�͈�s�ŋL�q���邱��
				uint8 param[8];
				int param_num;
				for(param_num=0; param_num<8 && str[ep]!='\0';){
					sp = ep;
					while(str[sp]==' ' || str[sp]=='\t' || str[sp]=='\r') sp++;
					if(str[sp]=='\n') break; // num==7�̎������Ŕ�����ΐ���
					ep = sp;
					while(str[ep]!=' ' && str[ep]!='\t' && str[ep]!='\r' && str[ep]!='\n' && str[ep]!='\0') ep++;
					param[param_num++] = strtol(str.substr(sp, ep-sp).c_str(), NULL, 16);
				}
				//printf("pn %d\n", param_num);

				if(brr_fname.substr(0,8)=="FF5inst:"){
					if(param_num==0){
						// ���w��
						tone_map[tone_id].octave = 0xFF; // E4 E7 E9 ���o�͂��Ȃ�
						tone_map[tone_id].transpose = 0;
						tone_map[tone_id].detune = 0;
					}
					else if(param_num==3){
						tone_map[tone_id].octave = param[0];
						tone_map[tone_id].transpose = param[1];
						tone_map[tone_id].detune = param[2];
					}
					else{
						printf("Error line %d : FF5�g�`�w��̏ꍇ�̃p�����[�^����0��3�ł�.\n", line);
						return -1;
					}
				}
				else{ // brr�t�@�C���w��̏ꍇ
					if(param_num==7){
						tone_map[tone_id].octave = param[0];
						tone_map[tone_id].transpose = param[1];
						tone_map[tone_id].detune = param[2];
						// adsr(EB EC ED EE)�A�p�����[�^��16�i��
						uint8 *adsr = param + 3;
						if(!(adsr[0]>=0 && adsr[0]<=0xF)){
							printf("Error line %d : Attack rate �� 0�`F �Ƃ��Ă�������.\n", line);
							return -1;
						}
						if(!(adsr[1]>=0 && adsr[1]<=7)){
							printf("Error line %d : Decay rate �� 0�`7 �Ƃ��Ă�������.\n", line);
							return -1;
						}
						if(!(adsr[2]>=0 && adsr[2]<=7)){
							printf("Error line %d : Sustain lebel �� 0�`7 �Ƃ��Ă�������.\n", line);
							return -1;
						}
						if(!(adsr[3]>=0 && adsr[3]<=0x1F)){
							printf("Error line %d : Sustain rate �� 00�`1F �Ƃ��Ă�������.\n", line);
							return -1;
						}
						// ADSR1 1dddaaaa
						spc.brr_map[brr_id].adsr1 = 0x80 | ((adsr[1]&7)<<4) | (adsr[0]&15);
						// ADSR2 lllsssss
						spc.brr_map[brr_id].adsr2 = ((adsr[2]&7)<<5) | (adsr[3]&0x1F);
					}
					else{
						printf("Error line %d : BRR�t�@�C���w��̏ꍇ��7�̃p�����[�^��ݒ肵�Ă�������.\n", line);
						return -1;
					}
				}

				// �풓�g�`�̏ꍇ��brr_id�͎g��Ȃ�
				if(!f_stayinst){
					tone_map[tone_id].brr_id = brr_id;
					brr_id++;
				}

				// �폜
				str.erase(p, ep-p);
				p--;
				continue;
			}

			// �g���b�N�ԍ��̎擾
			if(str.substr(p, 6)=="#track"){

				// �O�g���b�N�̏I������
				if(label_map.size()){
					map<string, int>::iterator mit;
					for(mit=label_map.begin(); mit!=label_map.end(); ++mit){
						if(label_set.find(mit->first)==label_set.end()){
							printf("Error track %d : �g���b�N���ŃW�����v���x�� %s ������܂���.\n", track_num, mit->first.c_str());
							return -1;
						}
					}
				}

				// #track ; �� #9
				if(str.substr(p, strlen(_MML_END_))==_MML_END_){
					str.replace(p, strlen(_MML_END_), "#9");
					break;
				}

				// �g���b�N������
				label_set.clear(); // �W�����v�惉�x���N���A
				label_map.clear(); // �W�����v���x���N���A

				int sp = skip_space(str, p+6);
				int ep = term_end(str, sp);
				track_num = atoi(str.substr(sp, ep-sp).c_str());
				if(!(track_num>=1 && track_num<=8)){
					printf("Error line %d : #track�i���o�[��1�`8�Ƃ��Ă�������.\n", line);
					return -1;
				}
				sprintf(buf, "#%d", track_num);
				str.replace(p, ep-p, buf);
				p++;
				continue;
			}

			// �}�N����`
			if(str.substr(p, 6)=="#macro"){
				// �}�N����`
				int sp = skip_space(str, p+6);
				int ep = term_end(str, sp);
				string macro_key = str.substr(sp, ep-sp);
				sp = str.find('"', ep) + 1;
				ep = str.find('"', sp);
				string macro_val = str.substr(sp, ep-sp);
				//printf("macro [%s][%s]\n", macro_key.c_str(), macro_val.c_str());
				str.erase(p, ep-p+1);
				p--;
				// �}�N���u��
				int lp = p;
				for(;;){
					sp = str.find(macro_key, lp);
					if(sp==string::npos) break;
					//printf("macro_val_line %d\n", line);
					ep = sp + macro_key.length();
					if(!isalnum(str[sp-1]) && !isalnum(str[ep])){
						str.replace(sp, ep-sp, macro_val);
					}
					lp = sp + macro_val.length();
				}
				continue;
			}

			printf("�x�� line %d : # ����`�̃R�}���h�ł�.\n", line);
			getchar();
			int ep = term_end(str, p);
			str.erase(p, ep-p);
			p--;
			continue;
		}

		// ���ʉ����ߍ���
		if(str[p]=='@' && str[p+1]=='@'){
			int sp = p + 2;
			int ep = term_end(str, sp);
			int id = atoi(str.substr(sp, ep-sp).c_str());
			if(id<0 || id>353){
				printf("Error line %d : @@���ʉ��ԍ��� 0�`353 �܂łł�.\n", line);
				return -1;
			}
			uint16 adrs = *(uint16*)(asd.eseq+id*2);
			if(adrs==0x0000){
				printf("Error line %d : @@%d ���ʉ��͂���܂���.\n", line, id);
				return -1;
			}
			adrs -= 0x2C00;
			string estr;
			int i;
			for(i=adrs; asd.eseq[i]!=0xF2; i++){
				sprintf(buf, "%02X ", asd.eseq[i]);
				estr += buf;
			}
			str.replace(p, ep-p, estr);
			p--;
			continue;
		}

		// �g�`�w��̎擾
		if(str[p]=='@'){ // @3 @12 @B @0C @piano
			int sp = p + 1;
			int ep = term_end(str, sp);
			string tone_id = str.substr(sp, ep-sp);
			if(tone_map.find(tone_id)==tone_map.end()){
				printf("Error line %d : @%s ��`����Ă��܂���.\n", line, tone_id.c_str());
				return -1;
			}
			tone_map[tone_id].used = true;
			// ADSR�͕ʂ̂Ƃ���ɖ��ߍ���
			// FF5inst��octave��0xFF�Ȃ琶�����Ȃ�
			char buf_octave[10], buf_transpose[10], buf_detune[10];
			if(tone_map[tone_id].brr_fname.substr(0,8)=="FF5inst:" && tone_map[tone_id].octave==0xFF){
				sprintf(buf_octave, "");
				sprintf(buf_transpose, "");
				sprintf(buf_detune, "");
			}
			else{
				sprintf(buf_octave, "E4 %02X", tone_map[tone_id].octave);
				sprintf(buf_transpose, "E7 %02X", tone_map[tone_id].transpose);
				sprintf(buf_detune, "E9 %02X", tone_map[tone_id].detune);
			}
			sprintf(buf, "EA %02X %s %s %s ",
				(tone_map[tone_id].brr_fname.substr(0,9)=="FF5inst:s")
					? tone_map[tone_id].inst_id : (0x20 + tone_map[tone_id].brr_id),
				buf_octave, buf_transpose, buf_detune
				);
			str.replace(p, ep-p, buf);
			p--;
			continue;
		}

		// ���[�v�̏���
		if(str[p]==']'){ // ���[�v�̌��
			int sp = skip_space(str, p+1);
			int ep = num_end(str, sp);
			int loop_count = atoi(str.substr(sp, ep-sp).c_str());
		//	printf("line %d  loop_count %d\n", line, loop_count); getchar();
			// "]" �� "F1 "
			str.replace(p, ep-p, "F1 ");
			// ���[�v�̐擪 [ ��������
			int break_dest = 0;
			int lp = p;
			for(lp=sp; lp>=0; lp--){
				if(str[lp]=='[') break;
				// ���[�v���̏����W�����v����
				if(str[lp]=='|'){
					str.insert(p+3, "break_dest ");
					sprintf(buf, "F9 %02X break_src ", (uint8)loop_count);
					str.replace(lp, 1, buf);
					// "|" �� "F9 XX break_src " + "break_dest "
					break_dest = (16-1) + 11;
				}
			}
			if(lp==-1){
				printf("Error line %d : ] �ɑΉ����� [ ������܂���.\n", line);
				return -1;
			}
			// "[" �� "F0 02 "
			sprintf(buf, "F0 %02X ", (uint8)(loop_count-1));
			str.replace(lp, 1, buf);
			p += (6-1) + (3-1) + break_dest;
			continue;
		}

		// 10�i������16�i���ɕϊ�
		if(str[p]=='d'){
			int sp = p + 1;
			int ep = term_end(str, sp);
			int d = atoi(str.substr(sp, ep-sp).c_str());
			sprintf(buf, "%02X", (uint8)d);
			str.replace(p, ep-sp+1, buf);
			p--;
			continue;
		}

		// �I�N�^�[�u����
		if(str[p]=='>'){
			if(f_octave_swap) sprintf(buf, "E6"); // �I�N�^�[�u -1
			else sprintf(buf, "E5"); // �I�N�^�[�u +1
			str.replace(p, 1, buf);
			p++;
			continue;
		}
		if(str[p]=='<'){
			if(f_octave_swap) sprintf(buf, "E5"); // �I�N�^�[�u +1
			else sprintf(buf, "E6"); // �I�N�^�[�u -1
			str.replace(p, 1, buf);
			p++;
			continue;
		}

		// �g���b�N���[�v
		if(str[p]=='L' && is_space(str[p-1]) && is_space(str[p+1])){
			continue;
		}

		// �W�����v�̏���
		if(str[p]=='J' && is_space(str[p-1]) && is_space(str[p+1])){
			int sp = skip_space(str, p+1);
			int ep = term_end(str, sp);
			string label_str = str.substr(sp, ep-sp);
		//	printf("[%s]\n", label_str.c_str());getchar();
			int label_num;
			if(label_map.find(label_str)==label_map.end()){
				label_num = label_map.size() + 1;
				label_map[label_str] = label_num;
			}
			else{
				label_num = label_map[label_str];
			}
			sprintf(buf, "FA jump_src_%d%d ", track_num, label_num);
			str.replace(p, ep-p, buf);
			p += 15 -1;
		//	str.insert(p,"p");
			continue;
		}
		if(str[p]==':'){ // jump dst
			int ep = p + 1;
			int sp = term_begin(str, p-1) +1;
			string label_str = str.substr(sp, ep-sp-1);
		//	printf("[%s] %d\n", label_str.c_str(), ep-sp-1);getchar();
			if(label_set.find(label_str)!=label_set.end()){
				printf("Error line %d : �W�����v���x�� %s �͂��łɒ�`����Ă��܂�.\n", line, label_str.c_str());
				return -1;
			}
			label_set.insert(label_str);
			int label_num;
			if(label_map.find(label_str)==label_map.end()){
				label_num = label_map.size() + 1;
				label_map[label_str] = label_num;
			}
			else{
				label_num = label_map[label_str];
			}
			sprintf(buf, "jump_dst_%d%d ", track_num, label_num);
			str.replace(sp, ep-sp, buf);
			p += -(ep-sp-1) + 12 -1;
		//	str.insert(p,"p");
			continue;
		}

		// �R���o�[�g�I��
		if(str[p]=='!'){
			str.replace(p, 1, "\n"_MML_END_" ");
			p--;
			continue;
		}
	}

	// ���g�ptone�̌x��
	map<string, FF5_TONE>::iterator mit;
	for(mit=tone_map.begin(); mit!=tone_map.end(); ++mit){
		if(mit->second.used==false){
			printf("Warning line %d : #tone %s \"%s\" �͎g�p����Ă��܂���.\n", mit->second.line, mit->first.c_str(), mit->second.brr_fname.c_str());
		//	getchar();
		}
	}

// �t�H�[�}�b�g�����������̂��e�X�g�o��
//FILE *fp=fopen("sample_debug.txt","w");fprintf(fp,str.c_str());fclose(fp);

	return 0;
}

int spcmake_byFF5::get_sequence(void)
{
	line = 1;

	const int SEQ_SIZE_MAX = 8*1024;
	uint8 seq[SEQ_SIZE_MAX];
	int seq_size = 0;

	int track_num = 0;
	int seq_id = -1;

	multimap<int, int> jump_src_map[8];
	map<int, int> jump_dst_map[8];

	int p;
	for(p=0; str[p]!='\0'; p++){
		if(seq_size>=SEQ_SIZE_MAX){
			printf("Error : �V�[�P���X�T�C�Y�� %d Byte �𒴂��Ă��܂��܂���.\n", SEQ_SIZE_MAX);
			return -1;
		}
		if(str[p]=='\n') line++;

		if(str[p]=='#'){
			//printf("t%d\n", track_num);
			// �g���b�N�̏I��
			if(track_num!=0){
				// �g���b�N���[�v������Ȃ�
				if(spc.track_loop[seq_id]!=0xFFFF){
				//	printf("track%d_loop %d\n", track_id, spc.track_loop[track_id]);
					seq[seq_size++] = 0xFA; // ���[�v�R�}���h
					// ���Βl�����Ă���
					seq[seq_size++] = (uint8)spc.track_loop[seq_id];
					seq[seq_size++] = spc.track_loop[seq_id] >> 8;
				//	printf("t%d seq_size %d\n", track_num, seq_size); getchar();
				}
				// F2�ŏI����ĂȂ��Ȃ�F2��u��
				else if(seq[seq_size-1]!=0xF2){
					seq[seq_size++] = 0xF2;
				}

				spc.seq[seq_id] = new uint8[seq_size];
				memcpy(spc.seq[seq_id], seq, seq_size);
				spc.seq_size[seq_id] = seq_size;
				//printf("size %d\n", seq_size);
			}

			track_num = str[p+1] - '0';
			//printf("track %d\n", track_num);
			if(track_num==9) break;

			// �g���b�N�̊J�n
			seq_id = track_num - 1;
			if(spc.seq_size[seq_id]!=0){
				printf("Error line %d : #track �g���b�N�i���o�[���d�����Ă��܂�.\n", line);
				return -1;
			}
			seq_size = 0;
			p++;
			continue;
		}
		if(track_num==0) continue;

		// �g���b�N���[�v�̌��o
		if(str[p]=='L'){
			if(spc.track_loop[seq_id]!=0xFFFF){ // ���ł� L ��������
				printf("Error line %d : L �̑��d�g�p�ł�.\n", line);
				return -1;
			}
		//	printf("t%d track_loop %d\n", track_num, seq_size);getchar();
			spc.track_loop[seq_id] = seq_size;
			continue;
		}

		// F9�R�}���h�̏���
		// F9 01 break_src  XX XX XXF1 break_dest 
		if(seq_size>=2 && seq[seq_size-2]==0xF9 && str.substr(p, 9)=="break_src"){
			int jp = 2; // F9����̑��΃A�h���X�A2�ō���
			int lp;
			for(lp=p+9; str[lp]!='#'; lp++){
				if(str.substr(lp, 10)=="break_dest"){
					str.erase(lp, 10); // break_dest�폜
					// break�摊�Βl�����Ă���
					jp--; // ���d���[�v����break��L���ɂ��邽�߂ɂ�F1�ɃW�����v����K�v������
					char buf[10];
					sprintf(buf, "%02X %02X ", (uint8)jp, (uint8)(jp>>8));
					str.replace(p, 9, buf); // break_src�u������
					spc.break_point[seq_id].push_back(seq_size); // break��A�h���X��u���ꏊ
					break;
				}
				if(is_cmd(str[lp])){
					jp++;
					lp++;
				}
			}
			p--;
			continue;
		}

		// �W�����v�̑O����
		if(str.substr(p, 5)=="jump_"){
			if(str.substr(p+5, 4)=="src_"){ // jump_src_NN
				int jump_id = atoi(str.substr(p+9, 2).c_str());
				jump_src_map[seq_id].insert(make_pair<int,int>(jump_id, seq_size));
				str.insert(p+11, "00 00 ");
				str.erase(p, 11);
				p--;
				continue;
			}
			if(str.substr(p+5, 4)=="dst_"){ // jump_dst_NN
				int jump_id = atoi(str.substr(p+9, 2).c_str());
				jump_dst_map[seq_id][jump_id] = seq_size;
				str.erase(p, 11);
				p--;
				continue;
			}
		}

		// ����R�}���h����
		if(is_cmd(str[p])){
			seq[seq_size++] = get_hex(str, p);
		//	printf("0x%02X ",get_hex(str, p));getchar();
			p++;
			continue;
		}
	}

	// �W�����v�̌㏈��
	int t;
	for(t=0; t<8; t++){
		// jump_src�̈ʒu�ɑ��΃W�����v�l������
		multimap<int, int>::iterator mit;
		for(mit=jump_src_map[t].begin(); mit!=jump_src_map[t].end(); ++mit){
			// jump_rel = jump_dst - jump_src
			*(uint16*)(spc.seq[t] + mit->second) = jump_dst_map[t][mit->first] - mit->second;
			// point��jump_src�̈ʒu������
			spc.jump_point[t].push_back(mit->second);
		}
	}

	// track���Ȃ��Ȃ�V�[�P���X�I����u���Ă���
	for(t=0; t<8; t++){
		if(spc.seq_size[t]==0){
			spc.seq[t] = new uint8[1];
			spc.seq[t][0] = 0xF2;
			spc.seq_size[t] = 1;
		}
	}
/*
	{
	int t, i;
	for(t=0; t<8; t++){
		printf("track%d\n", t+1);
		for(i=0; i<spc.seq_size[t]; i++){
			printf("%02X ", spc.seq[t][i]);
		}
		printf("\n");
	}
	printf("jp %d\n", spc.break_point[0][0]);
	getchar();
	}
*/
	return 0;
}

int spcmake_byFF5::get_ticks(int track)
{
	const uint8 *seq = spc.seq[track];

	int tick = 0;

	int ticks[15] = {192, 144, 96, 64, 72, 48, 32, 36, 24, 16, 12, 8, 6, 4, 3};

	int length[48] = { // 0xD0-0xFF
		1, 1, 2, 3, 2, 3, 3, 4, 1, 4, 1, 3, 1, 2, 1, 1,
		1, 1, 1, 1, 2, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 1,
		2, 1, 1, 2, 3, 2, 3, 3, 2, 4, 3, 3, 1, 1, 1, 1
	};

	int loop_depth = 0;
	int mul[10];

	int p = 0;
	for(;;){
		uint8 c = seq[p];
		if(c<=0xD1){
			int m = 1;
			for(int i=0; i<loop_depth; i++) m *= mul[i];
			tick += ticks[c % 15] * m;
			p++;
			continue;
		}
		if(c==0xF0){ // ���[�v�J�n
			mul[loop_depth++] = (uint32)seq[p+1] + 1;
		}
		else if(c==0xF9){ // �����W�����v
			mul[loop_depth-1]--;
		}
		else if(c==0xF1){ // ���[�v�I��
			loop_depth--;
		}
		else if(c==0xFA && (p+3)==spc.seq_size[track]){ // �g���b�N���[�v
			break;
		}
		else if(c==0xF2){ // �I��
			break;
		}
		p += length[c-0xD0];
	}

	return tick;
}

#include<time.h>

int spcmake_byFF5::make_spc(const char *spc_fname)
{
	// SPC Header
	uint8 header[0x100];
	memset(header, 0x00, 0x100);
	memcpy(header, "SNES-SPC700 Sound File Data v0.30", 33);
	header[0x21] = header[0x22] = header[0x23] = 26;
	header[0x24] = 30; // 0x1E

	// SPC700
	header[0x25] = 0x37; // PCL
	header[0x26] = 0x0E; // PCH
//	header[0x27] = 0x00; // A
//	header[0x28] = 0x00; // X
//	header[0x29] = 0x00; // Y
//	header[0x2A] = 0x00; // PSW
	header[0x2B] = 0xFD; // SP

	{
	// �o�C�i���t�H�[�}�b�g
	int i;
	for(i=0; i<32 && i<spc.songname.length(); i++) header[0x2E+i] = spc.songname[i];
	for(i=0; i<32 && i<spc.gametitle.length(); i++) header[0x4E+i] = spc.gametitle[i];
	for(i=0; i<32 && i<spc.artist.length(); i++) header[0xB0+i] = spc.artist[i];
	for(i=0; i<16 && i<spc.dumper.length(); i++) header[0x6E+i] = spc.dumper[i];
	if(spc.comment.length()){
		for(i=0; i<32 && i<spc.comment.length(); i++) header[0x7E+i] = spc.comment[i];
	}
	else{
		header[0x7E] = ' '; i = 1;
	}
	}

	// SPC�쐬�N����(16�i)
	time_t timer = time(NULL);
	struct tm *local;
	local = localtime(&timer); // �n�����ɕϊ�
	header[0x9E] = local->tm_mday; // ��
	header[0x9F] = local->tm_mon+1; // ��
	*(uint16*)(header+0xA0) = local->tm_year+1900; // �N

	// �Đ�����(3byte)
	*(uint32*)(header+0xA9) = spc.play_time;
	// �t�F�[�h�A�E�g����(4byte)
	*(uint32*)(header+0xAC) = spc.fade_time;
	
	// used to dump
	header[0xD1] = 4; // ETC


	// �T�E���h������
	uint8 *ram = new uint8[0x10000];
	memset(ram, 0x00, 0x10000);
	ram[0x007D] = 0x01; // �v���O�����֘A
	ram[0x007E] = 0xFF; // �J�n�^�C�~���O
	ram[0x0092] = 0x80; // ���ʉ��p��
	ram[0x009C] = 0x80; // �e���|
	ram[0x009E] = 0x80; // ���ʉ��e���|
	ram[0x00A8] = 0x80; // ����
//	ram[0x00B4] = 0x09; // ���ʉ��G�R�[
	ram[0x00B8] = 0xFF; // �{�����[���֘A
	ram[0x00C3] = 0x08; // �p��(����4bit) ok
	ram[0x00C4] = 0x0F; // �{�����[���֘A(����4bit) ok
	ram[0x00EB] = 0xFF; // �v���O�����֘A
	ram[0x00F1] = 0x01; // ctrl
	ram[0x00FA] = 0x24; // target0
	ram[0x01FE] = 0x8F; // �X�^�b�N
	ram[0x01FF] = 0x02; // �X�^�b�N

	// DSP������
	uint8 dsp_reg[128];
	memset(dsp_reg, 0x00, 128);
	dsp_reg[0x0C] = 0x7F; // MVOL_L
	dsp_reg[0x1C] = spc.f_surround ? 0x81 : 0x7F; // MVOL_R
//	dsp_reg[0x0D] = 0x64; // EFB �ݒ�ł��Ȃ��H
	dsp_reg[0x5D] = 0x1B; // DIR
//	dsp_reg[0x6D] = 0xD2; // ESA
//	dsp_reg[0x7D] = 0x05; // EDL
	// �G�R�[�o�b�t�@�̈�ݒ�
	uint16 echobuf_start_adrs = 0xFA00 - (spc.echo_depth << 11);
	dsp_reg[0x6D] = echobuf_start_adrs >> 8; // ESA
	dsp_reg[0x7D] = spc.echo_depth; // EDL


	// �x�[�X�A�h���X
	// rom�x�[�X����apu_ram�x�[�X�ɕύX���Ă����Ȃ�����
	*(uint16*)(ram+0x1C00) = 0x1C14;//0xE5FE;
	
	// �h���C�o
	memcpy(ram+0x0200, asd.driver, asd.driver_size);

	// �풓�g�`ADSR
	memcpy(ram+0x1A80, asd.sbrr_adsr, 16);
	// �풓�g�`�����␳
	memcpy(ram+0x1A00, asd.sbrr_tune, 16);
	// ���ʉ��V�[�P���X��
	if(spc.f_eseq_out){ // BRR�I�t�Z�b�g�w�薳����Ζ��ߍ���
		memcpy(ram+0x2C00, asd.eseq, asd.eseq_size);
	}

	// �����␳�AADSR���ߍ���
	int i;
	for(i=0; i<spc.brr_map.size(); i++){
		if(spc.brr_map[i].brr_fname.substr(0, 8)=="FF5inst:"){
		//	if(spc.brr_map[i].brr_fname[8]=='s') continue; // �풓�g�`��ADSR�ʓr
			int sp = 8;
			int ep = term_end(spc.brr_map[i].brr_fname, sp);
			int inst_id = strtol(spc.brr_map[i].brr_fname.substr(sp, ep-sp).c_str(), NULL, 16);
			*(uint16*)(ram+0x1A40+i*2) = asd.brr_tune[inst_id];
			*(uint16*)(ram+0x1AC0+i*2) = asd.brr_adsr[inst_id];
		}
		else{ // ����BRR�ł�ADSR�ݒ�
			*(uint16*)(ram+0x1A40+i*2) = 0x0000; // �����␳�l�ς��Ă��ω��Ȃ��H
			ram[0x1AC0+i*2  ] = spc.brr_map[i].adsr1;
			ram[0x1AC0+i*2+1] = spc.brr_map[i].adsr2;
		}
	}

	// 0x1D14����V�[�P���X�f�[�^
	uint16 seq_adrs[8];
	seq_adrs[0] = 0x1C14;
	seq_adrs[1] = seq_adrs[0] + spc.seq_size[0];
	seq_adrs[2] = seq_adrs[1] + spc.seq_size[1];
	seq_adrs[3] = seq_adrs[2] + spc.seq_size[2];
	seq_adrs[4] = seq_adrs[3] + spc.seq_size[3];
	seq_adrs[5] = seq_adrs[4] + spc.seq_size[4];
	seq_adrs[6] = seq_adrs[5] + spc.seq_size[5];
	seq_adrs[7] = seq_adrs[6] + spc.seq_size[6];
	uint16 seq_adrs_end = seq_adrs[7] + spc.seq_size[7];

	// �V�[�P���X�f�[�^�𖄂ߍ���
	for(i=0; i<8; i++){
		memcpy(ram + seq_adrs[i], spc.seq[i], spc.seq_size[i]);
	}
//	data[seq_adrs_end] = 0xF2; // �Ō�͕K�v�Ȃ�

	// BRR�ʒuauto
	if(spc.brr_offset==0xFFFF){
		spc.brr_offset = seq_adrs_end;
	}
	seq_adrs_end--;
	printf("SEQ end address 0x%04X\n", seq_adrs_end); //getchar();
	if(seq_adrs_end >= spc.brr_offset){
		printf("Error : #brr_offset 0x%04X ���V�[�P���X�Əd�Ȃ��Ă��܂�.\n", spc.brr_offset);
		delete[] ram;
		return -1;
	}
	if(spc.f_eseq_out && seq_adrs_end >= 0x2C00){
		printf("Error : �V�[�P���X�����ʉ��V�[�P���X�Əd�Ȃ��Ă��܂�.\n");
		delete[] ram;
		return -1;
	}

	uint16 seq_rom_adrs_base = *(uint16*)(ram+0x1C00);
//	printf("base_adrs 0x%04X\n", base_adrs);

	// �V�[�P���XROM�A�h���X�̐���
	uint16 seq_rom_adrs[8];
	seq_rom_adrs[0] = seq_rom_adrs_base;
	seq_rom_adrs[1] = seq_rom_adrs[0] + spc.seq_size[0];
	seq_rom_adrs[2] = seq_rom_adrs[1] + spc.seq_size[1];
	seq_rom_adrs[3] = seq_rom_adrs[2] + spc.seq_size[2];
	seq_rom_adrs[4] = seq_rom_adrs[3] + spc.seq_size[3];
	seq_rom_adrs[5] = seq_rom_adrs[4] + spc.seq_size[4];
	seq_rom_adrs[6] = seq_rom_adrs[5] + spc.seq_size[5];
	seq_rom_adrs[7] = seq_rom_adrs[6] + spc.seq_size[6];
	uint16 seq_rom_adrs_end = seq_rom_adrs[7] + spc.seq_size[7];
	// �V�[�P���XROM�A�h���X�̖��ߍ���
	for(i=0; i<8; i++){
		*(uint16*)(ram+0x1C02+i*2) = seq_rom_adrs[i];
	}
	*(uint16*)(ram+0x1C12) = seq_rom_adrs_end; // �V�[�P���X�A�h���X�G���h+1���w��

	// �W�����v����֘A
	{
	int t, i;

	// �g���b�N���[�v�A�h���X���ߍ���
	// FA XX XX
	for(t=0; t<8; t++){
	//	printf("seq%d_size %d\n", t, spc.seq_size[t]);
		if(spc.track_loop[t]!=0xFFFF){ // L������ꍇ
			for(i=spc.seq_size[t]-1; i>=0; i--){
				// �g���b�N�Ō��FA�ɑ΂��Ă̂݃��[�v�A�h���X�𒲐�
				if(spc.seq[t][i]==0xFA){
				//	printf("jump_rel %d %d\n", i, *(uint16*)(spc.seq[t]+i+1));
					uint16 jump_adrs = seq_rom_adrs[t] + *(uint16*)(spc.seq[t]+i+1);
				//	printf("jump_adrs 0x%04X\n", jump_adrs);
					*(uint16*)(ram+seq_adrs[t]+i+1) = jump_adrs;
					break; // 20191230
				}
			}
		}
	}

	// ���[�v�u���C�N�W�����v���ߍ���
	// F9 NN XX XX
	for(t=0; t<8; t++){
	//	printf("seq%d_size %d\n", t, spc.seq_size[t]);
		for(i=0; i<spc.break_point[t].size(); i++){
		//	printf("jump_rel %d  %d + %d\n", i, spc.break_point[t][i], *(uint16*)(spc.seq[t]+spc.break_point[t][i]));
			uint16 jump_adrs = seq_rom_adrs[t] + spc.break_point[t][i] + *(uint16*)(spc.seq[t]+spc.break_point[t][i]);
		//	printf("jump_adrs 0x%04X\n", jump_adrs);
			*(uint16*)(ram+seq_adrs[t]+spc.break_point[t][i]) = jump_adrs;
		}
	}

	// �W�����v�A�h���X���ߍ���
	// FA XX XX
	for(t=0; t<8; t++){
		for(i=0; i<spc.jump_point[t].size(); i++){
		//	printf("jump_rel %d  %d + %d\n", i, spc.jump_point[t][i], *(uint16*)(spc.seq[t]+spc.jump_point[t][i]));
			uint16 jump_adrs = seq_rom_adrs[t] + spc.jump_point[t][i] + *(uint16*)(spc.seq[t]+spc.jump_point[t][i]);
		//	printf("jump_adrs 0x%04X\n", jump_adrs);
			*(uint16*)(ram+seq_adrs[t]+spc.jump_point[t][i]) = jump_adrs;
		}
	}

	}

	// �풓�g�`BRR���ߍ��݁A�f�t�H���g��0x4800
//	memcpy(ram+0x4800, asd.sbrr, asd.sbrr_size);
	// �풓�g�`BRR���ߍ���
	memcpy(ram+spc.brr_offset, asd.sbrr, asd.sbrr_size);

	// �풓�g�`�A�h���X�ύX
	uint16 brr_adrs_sa = spc.brr_offset - 0x4800;
	for(i=0; i<8; i++){
		asd.sbrr_start[  i*2] += brr_adrs_sa;
		asd.sbrr_start[1+i*2] += brr_adrs_sa;
	}
	// �풓�g�`BRR�A�h���X���ߍ���
	memcpy(ram+0x1B00, asd.sbrr_start, 32);

	// BRR���ߍ���
	// ���łɖ��ߍ���BRR�͎g���܂킷
	map<string, pair<uint16, uint16> > brr_put_map;
	uint16 brr_offset = spc.brr_offset + asd.sbrr_size;
	uint32 adrs_index = 0;
	for(i=0; i<spc.brr_map.size(); i++){
		string brr_fname = spc.brr_map[i].brr_fname;
	//	if(brr_fname[0]=='\0') continue;
		uint32 start_adrs, loop_adrs;
		if(brr_put_map.find(brr_fname)==brr_put_map.end()){ // �u���ĂȂ�brr�Ȃ�

			int brr_size;
			uint8 *brr_data;
			if(brr_fname.substr(0, 8)=="FF5inst:"){ // FF5�̔g�`
			//	if(brr_fname[8]=='s') continue; // �풓�g�`�͔�΂�
				int sp = 8;
				int ep = term_end(brr_fname, sp);
				int inst_id = strtol(brr_fname.substr(sp, ep-sp).c_str(), NULL, 16);
				brr_size = asd.brr_size[inst_id] + 2; // �擪2�o�C�g���[�v�ǉ�
				brr_data = new uint8[brr_size];
				memcpy(brr_data+2, asd.brr[inst_id], asd.brr_size[inst_id]);
				*(uint16*)brr_data = asd.brr_loop[inst_id];
			//	printf("loop 0x%04X\n",asd.brr_loop[inst_id]);
			}
			else{ // BRR�t�@�C���w��
				FILE *brrfp = fopen(brr_fname.c_str(), "rb");
				if(brrfp==NULL){
					printf("Error : BRR�t�@�C�� %s ���J���܂���.\n", brr_fname.c_str());
					delete[] ram;
					return -1;
				}
				struct stat statBuf;
				if(stat(brr_fname.c_str(), &statBuf)==0) brr_size = statBuf.st_size;
			//	printf("brr_size %d\n", brr_size);
				brr_data = new uint8[brr_size];
				fread(brr_data, 1, brr_size, brrfp);
				fclose(brrfp);
			}

			// �{�Ƃ�0x3000�͌��ʉ��V�[�P���X��0x4800����BRR
			start_adrs = (uint32)brr_offset + adrs_index;
			loop_adrs = start_adrs + (uint32)(*(uint16*)brr_data);
//printf("%d %s start 0x%X end 0x%X\n", 0x20+i, brr_fname.c_str(), start_adrs, start_adrs+brr_size-2-1);
			if(start_adrs+brr_size-2-1 >= 0x0FA00){
				printf("BRR end address 0x%X\n", start_adrs+brr_size-2-1);
				printf("Error : %s BRR�G���A��0xFA00�𒴂��܂���.\n", brr_fname.c_str());
				delete[] brr_data;
				delete[] ram;
				return -1;
			}
			memcpy(ram+start_adrs, brr_data+2, brr_size-2);
			delete[] brr_data;

			if(start_adrs >= 0x0FA00){
				printf("BRR start address 0x%X\n", start_adrs);
				printf("Error : %s BRR�X�^�[�g�A�h���X��0xFA00�𒴂��܂���.\n", brr_fname.c_str());
				delete[] ram;
				return -1;
			}
			if(loop_adrs >= 0x0FA00){
				printf("BRR loop address 0x%X\n", loop_adrs);
				printf("Error : %s BRR���[�v�A�h���X��0xFA00�𒴂��܂���.\n", brr_fname.c_str());
				delete[] ram;
				return -1;
			}

			brr_put_map[brr_fname] = make_pair<uint16, uint16>(start_adrs, loop_adrs);
			adrs_index += brr_size -2;
		}
		else{ // ���łɔz�u����brr�Ȃ�A�h���X�𗬗p���邾��
			start_adrs = brr_put_map[brr_fname].first;
			loop_adrs = brr_put_map[brr_fname].second;
		}

		*(uint16*)(ram+0x1B80+i*4) = (uint16)start_adrs;
		*(uint16*)(ram+0x1B82+i*4) = (uint16)loop_adrs;
	}
	uint32 brr_adrs_end = (uint32)brr_offset + adrs_index -1;
	printf("BRR end address 0x%04X\n", brr_adrs_end); //getchar();
	printf("EchoBuf start   0x%04X\n", echobuf_start_adrs);//getchar();
	if(spc.f_brr_echo_overcheck){
		if((uint16)brr_adrs_end >= echobuf_start_adrs){
			printf("Error : BRR�f�[�^���G�R�[�o�b�t�@�̈�Əd�Ȃ��Ă��܂�.\n");
			delete[] ram;
			return -1;
		}
	}


	// SPC�o��
	FILE *ofp;
	ofp = fopen(spc_fname, "wb");
	if(ofp==NULL){
		printf("%s cant open.\n", spc_fname);
		return -1;
	}
	fwrite(header, 1, 0x100, ofp);
	fwrite(ram, 1, 0x10000, ofp);
	fwrite(dsp_reg, 1, 128, ofp);
	fclose(ofp);
	printf("%s �𐶐����܂���.\n\n", spc_fname);
/*
	ofp = fopen("out.bin", "wb");
	if(ofp==NULL){
		printf("out.bin dont open\n");
		return -1;
	}
	fwrite(data+0x100, 1, total_size-0x100, ofp);
	fclose(ofp);
*/
/*
	uint8 *rom = new uint8[2*1024*1024];
	memset(rom, 0x00, 2*1024*1024);
	memcpy(rom+seq_rom_adrs_base, ram+0x1C14, 0x10000-0x1C14);
	FILE *romfp = fopen("sample_debug.rom", "wb");
	fwrite(rom, 1, 2*1024*1024, romfp);
	fclose(romfp);
	delete[] rom;
*/

	delete[] ram;

	return 0;
}

#include "brr2wav.cpp"

int main(int argc, char *argv[])
{
	printf("[ spcmake_byFF5 ver.20201003 ]\n\n");

#ifdef _DEBUG
	argc = 5;
	argv[1] = "-i";
	argv[2] = "sample.txt";
	argv[3] = "-o";
	argv[4] = "sample.spc";
#endif

	if(argc<5){
		printf("usage : spcmake_byFF5.exe -i input.txt -o output.spc\n");
		getchar();
		return -1;
	}

	char *input_fname = NULL;
	char *output_fname = NULL;
	bool f_ticks = false;
	bool f_brr2wav = false;

	int argi;
	for(argi=1; argi<argc; argi++){
		if(strncmp(argv[argi], "-i", 2)==0){
			input_fname = argv[argi+1];
			argi++;
		}
		else if(strncmp(argv[argi], "-o", 2)==0){
			output_fname = argv[argi+1];
			if(strncmp(output_fname+strlen(output_fname)-4, ".spc", 4)!=0){
				printf("Error : -o %s �o�̓t�@�C������.spc�ł͂���܂���.\n", output_fname);
				return -1;
			}
			argi++;
		}
		else if(strncmp(argv[argi], "-ticks", 6)==0){
			f_ticks = true;
		}
		else if(strncmp(argv[argi], "-brr2wav", 8)==0){
			f_brr2wav = true;
		}
		else{
			printf("�s���ȃI�v�V�����H %s \n", argv[argi]);
			getchar();
			return -1;
		}
	}

	spcmake_byFF5 spcmakeff5;

	if(spcmakeff5.asd.get_akao("FinalFantasy5.rom")){
		return -1;
	}

	if(f_brr2wav){
		system("mkdir brr2wav");
		int i;
		for(i=0; i<FF5_BRR_NUM; i++){
			char wav_fname[40];
			sprintf(wav_fname, "brr2wav/FF5_%02X.wav", i);
			brr2wav(wav_fname, spcmakeff5.asd.brr[i], spcmakeff5.asd.brr_size[i], spcmakeff5.asd.brr_loop[i], 0x1000);
		}
	}

	if(spcmakeff5.read_mml(input_fname)){
		printf("\n");
		return -1;
	}

	if(spcmakeff5.formatter()){
		printf("\n");
#ifdef _DEBUG
	getchar();
#endif
		return -1;
	}

//fp=fopen("out.txt","w");fprintf(fp,str.c_str());fclose(fp);
	printf("\n");
	printf("songname[%s]\n", spcmakeff5.spc.songname.c_str());
	printf("dumper[%s]\n", spcmakeff5.spc.dumper.c_str());
	printf("\n");

	if(spcmakeff5.get_sequence()){
		printf("\n");
		return -1;
	}

	if(spcmakeff5.make_spc(output_fname)){
		printf("\n");
		return -1;
	}

	if(f_ticks){
		printf("\n");
		int i;
		for(i=0; i<8; i++){
			printf("  track%d %6d ticks\n", i+1, spcmakeff5.get_ticks(i));
		}
		getchar();
	}

//	getchar();

	return 0;
}
