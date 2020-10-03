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

	// 効果音シーケンス
	uint32 eseq_size;
	uint8 *eseq;

	// 常駐波形
	uint32 sbrr_size;
	uint8 *sbrr;
	uint16 sbrr_start[16]; // スタートアドレスとループアドレス
	uint16 sbrr_tune[8];
	uint16 sbrr_adsr[8];

	// 波形
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

// FinalFantasy5.romからAkaoサウンドドライバ等取得
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
		printf("FF5のromサイズが違う？ヘッダが付いてる？\n");
		return -1;
	}
	uint8 *rom = new uint8[2*1024*1024];
	fread(rom, 1, 2*1024*1024, fp);
	fclose(fp);

	// FF5のROMか確認
	if(rom[0]!=0x78){
		delete[] rom;
		printf("FF5のromにヘッダが付いているようです？\n");
		return -1;
	}
	if(!(rom[0x0FFC0]=='F' && rom[0x0FFC6]=='F' && rom[0x0FFCE]=='5')){
		delete[] rom;
		printf("FF5のromではない？\n");
		return -1;
	}

/*
音源使用は32個まで

0x0000 - 0x01FF　ワークメモリ
0x0200 - 0x19EF　サウンドドライバ
0x1A00 - 0x1A0F　常駐波形音程補正
0x1A40 - 0x1A7F　音源音程補正
0x1A80 - 0x1A8F　常駐波形ADSR
0x1AC0 - 0x1AFF　音源ADSR
0x1B00 - 0x1B1F　常駐波形BRRアドレス
0x1B80 - 0x1BFF　音源BRRアドレス
0x1C00 - 0xXXXX　シーケンス
0x2C00 - 0x47FF　効果音シーケンスと効果音BRR（無くてもいい）
0x4800 - 0x490D　常駐波形BRR（移動ok）
0x490E - 0xXXXX　音源BRR（移動ok）
0xD200 - 0xF9FF　エコーバッファ（移動ok）
0xFA00 - 0xFFFF　ワークメモリ
*/

	// サウンドドライバ
	// 0x04064D 2バイトはサイズ
	// 0x04064F - 0x041E3E -> 0x0200 - 0x19EF
	driver_size = *(uint16*)(rom+0x4064D);
	driver = new uint8[driver_size];
	memcpy(driver, rom+0x4064F, driver_size);
//FILE *fp=fopen("out.bin","wb");fwrite(audio.driver,1,audio.driver_size,fp);fclose(fp);


	// 常駐波形BRR
	// 0x041E3F 2バイトはサイズ(0x010E)
	// 0x041E41 - 0x041F4E -> 0x4800 - 0x490D
	sbrr_size = *(uint16*)(rom+0x41E3F);
	sbrr = new uint8[sbrr_size];
	memcpy(sbrr, rom+0x41E41, sbrr_size);
//{FILE *fp=fopen("out.bin","wb");fwrite(asd.sbrr,1,asd.sbrr_size,fp);fclose(fp);}

	// 常駐波形BRRアドレス
	// 0x041F4F 2バイトはサイズ
	// 0x041F51 - 0x041F70 -> 0x1B00 - 0x1B1F
	// 0x4800-0x48FCを指す
	memcpy(sbrr_start, rom+0x41F51, 32); // (2+2)byte x 8

	// 常駐波形ADSR
	// 0x041F71 2バイトはサイズ
	// 0x041F73 - 0x041F82 -> 0x1A80 - 0x1A8F
	memcpy(sbrr_adsr, rom+0x41F73, 16); // 2byte x 8

	// 常駐波形音程補正
	// 0x041F83 2バイトはサイズ
	// 0x041F85 - 0x041F94 -> 0x1A00 - 0x1A0F
	memcpy(sbrr_tune, rom+0x41F85, 16); // 2byte x 8


	// 音源BRR
	// 0x043C6F - 0x043CD7 -> 0x490E ～ 必要な場所まで
	int i;
	for(i=0; i<FF5_BRR_NUM; i++){
		// 先頭2バイトはサイズ
		int brr_adrs = *(uint32*)(rom+0x43C6F+i*3) & 0x001FFFFF;
		brr_size[i] = *(uint16*)(rom+brr_adrs);
		brr[i] = new uint8[brr_size[i]];
		memcpy(brr[i], rom+brr_adrs+2, brr_size[i]);
	}
	// 音源ループ
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
	// 音源音程補正
	// 0x043D1E - 0x043D63 -> 0x1A40 - 0x1A7F
	memcpy(brr_tune, rom+0x43D1E, 2*FF5_BRR_NUM); // 2byte x 35

	// 音源ADSR
	// 0x043D64 - 0x043DA9 -> 0x1AC0 - 0x1AFF
	memcpy(brr_adsr, rom+0x43D64, 2*FF5_BRR_NUM); // 2byte x 35


	// 効果音シーケンスと効果音BRR
	// 0x041F95 2バイトはサイズ
	// 0x041F97 - 0x043B96 -> 0x2C00 - 0x47FF
	// 0x2C00～効果音シーケンスアドレス
	// 0x3000～効果音シーケンス
	eseq_size = *(uint16*)(rom+0x41F95);
	eseq = new uint8[eseq_size];
	memcpy(eseq, rom+0x41F97, eseq_size);
	eseq[339*2] = 0xBB; // 効果音339は幻？
//uint8 buf[0x2C00];memset(buf,0x00,0x2C00);
//FILE *fp=fopen("out.bin","wb");fwrite(buf,1,0x2C00,fp);fwrite(effect_seq,1,size,fp);fclose(fp);
/*
// 効果音シーケンス出力テスト
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
	int inst_id; // 常駐波形 only
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

	uint16 track_loop[8]; // 0xFA 0xFFFFの場合はトラックループなし
	vector<uint16> break_point[8]; // 0xF9
	vector<uint16> jump_point[8]; // 0xFA

	uint16 brr_offset;
	bool f_brr_echo_overcheck;
	uint16 echo_depth;
	bool f_surround; // 逆位相サラウンド
	bool f_eseq_out; // 効果音シーケンス埋め込み

	FF5_SPC()
	{
		play_time = 300; // 秒、デフォルト再生時間
		fade_time = 10000; // ミリ秒、デフォルトフェードアウト時間
		int i;
		for(i=0; i<8; i++){
			seq[i] = NULL;
			seq_size[i] = 0;
			track_loop[i] = 0xFFFF;
		}
		// 0x2C00 or 0x3000 or 0x4800からBRR, 最初の0x10E(0x10Dまで)は常駐波形BRR
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
	// シーケンスファイル読み込み
	FILE *fp = fopen(mml_fname, "r");
	if(fp==NULL){
		printf("HexMMLファイル %s が開けません.\n", mml_fname);
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
		printf("Error line %d : 16進数表記異常です.\n", line);
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

	// コメント削除
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
	// 最後に _MML_END_ を付加
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

			// 曲名の取得
			if(str.substr(p, 9)=="#songname"){
				int sp = str.find('"', p+9) + 1;
				int ep = str.find('"', sp);
				spc.songname = str.substr(sp, ep-sp);
				if(spc.songname.length()>32){
					printf("警告 line %d : #songnameが32バイトを超えています.\n", line);
					getchar();
				}
				str.erase(p, ep-p+1);
				p--;
				continue;
			}

			// ゲーム名の取得
			if(str.substr(p, 10)=="#gametitle"){
				int sp = str.find('"', p+10) + 1;
				int ep = str.find('"', sp);
				spc.gametitle = str.substr(sp, ep-sp);
				if(spc.gametitle.length()>32){
					printf("警告 line %d : #gametitleが32バイトを超えています.\n", line);
					getchar();
				}
				str.erase(p, ep-p+1);
				p--;
				continue;
			}

			// 作曲者の取得
			if(str.substr(p, 7)=="#artist"){
				int sp = str.find('"', p+7) + 1;
				int ep = str.find('"', sp);
				spc.artist = str.substr(sp, ep-sp);
				if(spc.artist.length()>32){
					printf("警告 line %d : #artistが32バイトを超えています.\n", line);
					getchar();
				}
				str.erase(p, ep-p+1);
				p--;
				continue;
			}

			// 作成者の取得
			if(str.substr(p, 7)=="#dumper"){
				int sp = str.find('"', p+7) + 1;
				int ep = str.find('"', sp);
				spc.dumper = str.substr(sp, ep-sp);
				if(spc.dumper.length()>16){
					printf("警告 line %d : #dumperが16バイトを超えています.\n", line);
					getchar();
				}
				str.erase(p, ep-p+1);
				p--;
				continue;
			}

			// コメントの取得
			if(str.substr(p, 8)=="#comment"){
				int sp = str.find('"', p+8) + 1;
				int ep = str.find('"', sp);
				spc.comment = str.substr(sp, ep-sp);
				if(spc.comment.length()>32){
					printf("警告 line %d : #commentが32バイトを超えています.\n", line);
					getchar();
				}
				str.erase(p, ep-p+1);
				p--;
				continue;
			}

			// 再生時間とフェードアウト時間の設定
			if(str.substr(p, 7)=="#length"){
				// 再生時間
				int sp = skip_space(str, p+7);
				int ep = term_end(str, sp);
				string sec_str = str.substr(sp, ep-sp);
				int cp;
				if((cp=sec_str.find(':'))!=string::npos){ // 例：2:30
					sec_str[cp] = '\0';
					spc.play_time = atoi(sec_str.c_str()) * 60 + atoi(sec_str.c_str()+cp+1);
				}
				else{ // 例：150
					spc.play_time = atoi(sec_str.c_str());
				}

				// フェードアウト時間
				sp = skip_space(str, ep);
				ep = term_end(str, sp);
				spc.fade_time = atoi(str.substr(sp, ep-sp).c_str());
				
				str.erase(p, ep-p);
				p--;
				continue;
			}

			// BRRオフセット
			if(str.substr(p, 11)=="#brr_offset"){

				// オフセット時は効果音シーケンスは出力しない
				spc.f_eseq_out = false;

				int sp = skip_space(str, p+11);
				int ep = term_end(str, sp);
				if(str.substr(sp, ep-sp)=="auto"){
					spc.brr_offset = 0xFFFF;
				}
				else{ // 0x3000 とか
					int brr_offset = strtol(str.substr(sp, ep-sp).c_str(), NULL, 16);
				//	printf("brr_offset 0x%04X\n", brr_offset);
					if(brr_offset<0 || brr_offset>=0x10000){
						printf("Error line %d : #brr_offsetの値が不正です.\n", line);
						return -1;
					}
					spc.brr_offset = brr_offset;
				}
				str.erase(p, ep-p);
				p--;
				continue;
			}

			// BRR領域がエコーバッファに重なるのチェックを有効にする
			if(str.substr(p, 19)=="#brr_echo_overcheck"){
				spc.f_brr_echo_overcheck = true;
				str.erase(p, 19);
				p--;
				continue;
			}

			// エコー深さ指定
			if(str.substr(p, 11)=="#echo_depth"){
				int sp = skip_space(str, p+11);
				if(str[sp]=='#'){
					printf("Error : #echo_depth パラメータを指定してください.\n");
					return -1;
				}
				int ep = term_end(str, sp);
				// EDL default 5
				int echo_depth = atoi(str.substr(sp, ep-sp).c_str());
				if(echo_depth<=0 || echo_depth>=16){
					printf("Error line %d : #echo_depth は 1～15 としてください.\n", line);
					return -1;
				}
				spc.echo_depth = echo_depth;
				str.erase(p, ep-p);
				p--;
				continue;
			}

			// 逆位相サラウンド有効
			if(str.substr(p, 9)=="#surround"){
				spc.f_surround = true;
				str.erase(p, 9);
				p--;
				continue;
			}

			// オクターブコマンド入れ替え
			if(str.substr(p, 7)=="#swap<>" || str.substr(p, 7)=="#swap><"){
				f_octave_swap ^= 1;
				str.erase(p, 7);
				p--;
				continue;
			}

			// tone_idを省略時に自動割り当て
			if(str.substr(p, 19)=="#auto_assign_toneid"){
				f_auto_assign_toneid = true;
				str.erase(p, 19);
				p--;
				continue;
			}

			// 波形宣言
			if(str.substr(p, 5)=="#tone"){
				int sp = skip_space(str, p+5); // tone指定先頭
				if(str[sp]=='"'){
					// tone_id省略されている場合は内部的に割り当てる
					if(f_auto_assign_toneid){
						static int auto_id = 32;
						char id_buf[10];
						sprintf(id_buf, "%d ", auto_id++);
						str.insert(sp, id_buf);
					}
					else{
						printf("Error line %d : #tone の tone_id が指定されていません.\n", line);
						return -1;
					}
				}
				int ep = term_end(str, sp);
				string tone_id = str.substr(sp, ep-sp);
				//printf("tone_id[%s]\n",tone_id.c_str());getchar();
				if(tone_map.find(tone_id)!=tone_map.end()){
					printf("Error line %d : #tone %s はすでに宣言されています.\n", line, tone_id.c_str());
					return -1;
				}
				// 常駐波形でもオクターブ・トランスポーズ・ディチューン設定するから追加
				tone_map[tone_id].used = false; // tone追加
				tone_map[tone_id].line = line;

				// brr_fnameの取得
				sp = skip_space(str, ep) + 1;
				ep = term_end(str, sp);
				string brr_fname = str.substr(sp, ep-sp-1);
				// 常駐波形ならtrue
				bool f_stayinst = brr_fname.substr(0,9)=="FF5inst:s";
				if(brr_fname.substr(0,8)=="FF5inst:"){
					int ssp = f_stayinst ? 9 : 8;
					int eep = term_end(brr_fname, ssp);
					int inst_id = strtol(brr_fname.substr(ssp, eep-ssp).c_str(), NULL, 16);
					if(!f_stayinst && (inst_id<0 || inst_id>=FF5_BRR_NUM)){
						printf("Error line %d : FF5inst 波形指定は 00～22(16進数) としてください.\n", line);
						return -1;
					}
					if(f_stayinst && (inst_id<0 || inst_id>7)){
						printf("Error line %d : FF5inst 常駐波形指定は s0～s7 としてください.\n", line);
						return -1;
					}
					tone_map[tone_id].inst_id = inst_id;
				}
				else{
					struct stat st;
					if(stat(brr_fname.c_str(), &st)!=0){
						printf("Error line %d : BRRファイル %s がありません.\n", line, brr_fname.c_str());
						return -1;
					}
					if((st.st_size-2)%9){
						printf("Error line %d : BRRファイル %s サイズ異常です.ループアドレスが付加されていない？\n", line, brr_fname.c_str());
						return -1;
					}
				}
				// 常駐波形じゃなければBRRを追加する
				if(!f_stayinst){
					if(brr_id>=32){
						printf("Error line %d : tone宣言(BRR指定)は32個までです.\n", line);
						return -1;
					}
					spc.brr_map[brr_id].brr_fname = brr_fname;
				}
				tone_map[tone_id].brr_fname = brr_fname;

				// パラメータ取得、#toneは一行で記述すること
				uint8 param[8];
				int param_num;
				for(param_num=0; param_num<8 && str[ep]!='\0';){
					sp = ep;
					while(str[sp]==' ' || str[sp]=='\t' || str[sp]=='\r') sp++;
					if(str[sp]=='\n') break; // num==7の時ここで抜ければ正常
					ep = sp;
					while(str[ep]!=' ' && str[ep]!='\t' && str[ep]!='\r' && str[ep]!='\n' && str[ep]!='\0') ep++;
					param[param_num++] = strtol(str.substr(sp, ep-sp).c_str(), NULL, 16);
				}
				//printf("pn %d\n", param_num);

				if(brr_fname.substr(0,8)=="FF5inst:"){
					if(param_num==0){
						// 無指定
						tone_map[tone_id].octave = 0xFF; // E4 E7 E9 を出力しない
						tone_map[tone_id].transpose = 0;
						tone_map[tone_id].detune = 0;
					}
					else if(param_num==3){
						tone_map[tone_id].octave = param[0];
						tone_map[tone_id].transpose = param[1];
						tone_map[tone_id].detune = param[2];
					}
					else{
						printf("Error line %d : FF5波形指定の場合のパラメータ数は0個か3個です.\n", line);
						return -1;
					}
				}
				else{ // brrファイル指定の場合
					if(param_num==7){
						tone_map[tone_id].octave = param[0];
						tone_map[tone_id].transpose = param[1];
						tone_map[tone_id].detune = param[2];
						// adsr(EB EC ED EE)、パラメータは16進数
						uint8 *adsr = param + 3;
						if(!(adsr[0]>=0 && adsr[0]<=0xF)){
							printf("Error line %d : Attack rate は 0～F としてください.\n", line);
							return -1;
						}
						if(!(adsr[1]>=0 && adsr[1]<=7)){
							printf("Error line %d : Decay rate は 0～7 としてください.\n", line);
							return -1;
						}
						if(!(adsr[2]>=0 && adsr[2]<=7)){
							printf("Error line %d : Sustain lebel は 0～7 としてください.\n", line);
							return -1;
						}
						if(!(adsr[3]>=0 && adsr[3]<=0x1F)){
							printf("Error line %d : Sustain rate は 00～1F としてください.\n", line);
							return -1;
						}
						// ADSR1 1dddaaaa
						spc.brr_map[brr_id].adsr1 = 0x80 | ((adsr[1]&7)<<4) | (adsr[0]&15);
						// ADSR2 lllsssss
						spc.brr_map[brr_id].adsr2 = ((adsr[2]&7)<<5) | (adsr[3]&0x1F);
					}
					else{
						printf("Error line %d : BRRファイル指定の場合は7個のパラメータを設定してください.\n", line);
						return -1;
					}
				}

				// 常駐波形の場合はbrr_idは使わない
				if(!f_stayinst){
					tone_map[tone_id].brr_id = brr_id;
					brr_id++;
				}

				// 削除
				str.erase(p, ep-p);
				p--;
				continue;
			}

			// トラック番号の取得
			if(str.substr(p, 6)=="#track"){

				// 前トラックの終了処理
				if(label_map.size()){
					map<string, int>::iterator mit;
					for(mit=label_map.begin(); mit!=label_map.end(); ++mit){
						if(label_set.find(mit->first)==label_set.end()){
							printf("Error track %d : トラック内でジャンプラベル %s がありません.\n", track_num, mit->first.c_str());
							return -1;
						}
					}
				}

				// #track ; → #9
				if(str.substr(p, strlen(_MML_END_))==_MML_END_){
					str.replace(p, strlen(_MML_END_), "#9");
					break;
				}

				// トラック初期化
				label_set.clear(); // ジャンプ先ラベルクリア
				label_map.clear(); // ジャンプラベルクリア

				int sp = skip_space(str, p+6);
				int ep = term_end(str, sp);
				track_num = atoi(str.substr(sp, ep-sp).c_str());
				if(!(track_num>=1 && track_num<=8)){
					printf("Error line %d : #trackナンバーは1～8としてください.\n", line);
					return -1;
				}
				sprintf(buf, "#%d", track_num);
				str.replace(p, ep-p, buf);
				p++;
				continue;
			}

			// マクロ定義
			if(str.substr(p, 6)=="#macro"){
				// マクロ定義
				int sp = skip_space(str, p+6);
				int ep = term_end(str, sp);
				string macro_key = str.substr(sp, ep-sp);
				sp = str.find('"', ep) + 1;
				ep = str.find('"', sp);
				string macro_val = str.substr(sp, ep-sp);
				//printf("macro [%s][%s]\n", macro_key.c_str(), macro_val.c_str());
				str.erase(p, ep-p+1);
				p--;
				// マクロ置換
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

			printf("警告 line %d : # 未定義のコマンドです.\n", line);
			getchar();
			int ep = term_end(str, p);
			str.erase(p, ep-p);
			p--;
			continue;
		}

		// 効果音埋め込み
		if(str[p]=='@' && str[p+1]=='@'){
			int sp = p + 2;
			int ep = term_end(str, sp);
			int id = atoi(str.substr(sp, ep-sp).c_str());
			if(id<0 || id>353){
				printf("Error line %d : @@効果音番号は 0～353 までです.\n", line);
				return -1;
			}
			uint16 adrs = *(uint16*)(asd.eseq+id*2);
			if(adrs==0x0000){
				printf("Error line %d : @@%d 効果音はありません.\n", line, id);
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

		// 波形指定の取得
		if(str[p]=='@'){ // @3 @12 @B @0C @piano
			int sp = p + 1;
			int ep = term_end(str, sp);
			string tone_id = str.substr(sp, ep-sp);
			if(tone_map.find(tone_id)==tone_map.end()){
				printf("Error line %d : @%s 定義されていません.\n", line, tone_id.c_str());
				return -1;
			}
			tone_map[tone_id].used = true;
			// ADSRは別のところに埋め込む
			// FF5instでoctaveが0xFFなら生成しない
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

		// ループの処理
		if(str[p]==']'){ // ループの後ろ
			int sp = skip_space(str, p+1);
			int ep = num_end(str, sp);
			int loop_count = atoi(str.substr(sp, ep-sp).c_str());
		//	printf("line %d  loop_count %d\n", line, loop_count); getchar();
			// "]" → "F1 "
			str.replace(p, ep-p, "F1 ");
			// ループの先頭 [ を見つける
			int break_dest = 0;
			int lp = p;
			for(lp=sp; lp>=0; lp--){
				if(str[lp]=='[') break;
				// ループ中の条件ジャンプ処理
				if(str[lp]=='|'){
					str.insert(p+3, "break_dest ");
					sprintf(buf, "F9 %02X break_src ", (uint8)loop_count);
					str.replace(lp, 1, buf);
					// "|" → "F9 XX break_src " + "break_dest "
					break_dest = (16-1) + 11;
				}
			}
			if(lp==-1){
				printf("Error line %d : ] に対応する [ がありません.\n", line);
				return -1;
			}
			// "[" → "F0 02 "
			sprintf(buf, "F0 %02X ", (uint8)(loop_count-1));
			str.replace(lp, 1, buf);
			p += (6-1) + (3-1) + break_dest;
			continue;
		}

		// 10進数から16進数に変換
		if(str[p]=='d'){
			int sp = p + 1;
			int ep = term_end(str, sp);
			int d = atoi(str.substr(sp, ep-sp).c_str());
			sprintf(buf, "%02X", (uint8)d);
			str.replace(p, ep-sp+1, buf);
			p--;
			continue;
		}

		// オクターブ操作
		if(str[p]=='>'){
			if(f_octave_swap) sprintf(buf, "E6"); // オクターブ -1
			else sprintf(buf, "E5"); // オクターブ +1
			str.replace(p, 1, buf);
			p++;
			continue;
		}
		if(str[p]=='<'){
			if(f_octave_swap) sprintf(buf, "E5"); // オクターブ +1
			else sprintf(buf, "E6"); // オクターブ -1
			str.replace(p, 1, buf);
			p++;
			continue;
		}

		// トラックループ
		if(str[p]=='L' && is_space(str[p-1]) && is_space(str[p+1])){
			continue;
		}

		// ジャンプの処理
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
				printf("Error line %d : ジャンプラベル %s はすでに定義されています.\n", line, label_str.c_str());
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

		// コンバート終了
		if(str[p]=='!'){
			str.replace(p, 1, "\n"_MML_END_" ");
			p--;
			continue;
		}
	}

	// 未使用toneの警告
	map<string, FF5_TONE>::iterator mit;
	for(mit=tone_map.begin(); mit!=tone_map.end(); ++mit){
		if(mit->second.used==false){
			printf("Warning line %d : #tone %s \"%s\" は使用されていません.\n", mit->second.line, mit->first.c_str(), mit->second.brr_fname.c_str());
		//	getchar();
		}
	}

// フォーマット処理したものをテスト出力
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
			printf("Error : シーケンスサイズが %d Byte を超えてしまいました.\n", SEQ_SIZE_MAX);
			return -1;
		}
		if(str[p]=='\n') line++;

		if(str[p]=='#'){
			//printf("t%d\n", track_num);
			// トラックの終了
			if(track_num!=0){
				// トラックループがあるなら
				if(spc.track_loop[seq_id]!=0xFFFF){
				//	printf("track%d_loop %d\n", track_id, spc.track_loop[track_id]);
					seq[seq_size++] = 0xFA; // ループコマンド
					// 相対値を入れておく
					seq[seq_size++] = (uint8)spc.track_loop[seq_id];
					seq[seq_size++] = spc.track_loop[seq_id] >> 8;
				//	printf("t%d seq_size %d\n", track_num, seq_size); getchar();
				}
				// F2で終わってないならF2を置く
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

			// トラックの開始
			seq_id = track_num - 1;
			if(spc.seq_size[seq_id]!=0){
				printf("Error line %d : #track トラックナンバーが重複しています.\n", line);
				return -1;
			}
			seq_size = 0;
			p++;
			continue;
		}
		if(track_num==0) continue;

		// トラックループの検出
		if(str[p]=='L'){
			if(spc.track_loop[seq_id]!=0xFFFF){ // すでに L があった
				printf("Error line %d : L の多重使用です.\n", line);
				return -1;
			}
		//	printf("t%d track_loop %d\n", track_num, seq_size);getchar();
			spc.track_loop[seq_id] = seq_size;
			continue;
		}

		// F9コマンドの処理
		// F9 01 break_src  XX XX XXF1 break_dest 
		if(seq_size>=2 && seq[seq_size-2]==0xF9 && str.substr(p, 9)=="break_src"){
			int jp = 2; // F9からの相対アドレス、2で合う
			int lp;
			for(lp=p+9; str[lp]!='#'; lp++){
				if(str.substr(lp, 10)=="break_dest"){
					str.erase(lp, 10); // break_dest削除
					// break先相対値を入れておく
					jp--; // 多重ループ内でbreakを有効にするためにはF1にジャンプする必要がある
					char buf[10];
					sprintf(buf, "%02X %02X ", (uint8)jp, (uint8)(jp>>8));
					str.replace(p, 9, buf); // break_src置き換え
					spc.break_point[seq_id].push_back(seq_size); // break先アドレスを置く場所
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

		// ジャンプの前処理
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

		// 制御コマンド処理
		if(is_cmd(str[p])){
			seq[seq_size++] = get_hex(str, p);
		//	printf("0x%02X ",get_hex(str, p));getchar();
			p++;
			continue;
		}
	}

	// ジャンプの後処理
	int t;
	for(t=0; t<8; t++){
		// jump_srcの位置に相対ジャンプ値を入れる
		multimap<int, int>::iterator mit;
		for(mit=jump_src_map[t].begin(); mit!=jump_src_map[t].end(); ++mit){
			// jump_rel = jump_dst - jump_src
			*(uint16*)(spc.seq[t] + mit->second) = jump_dst_map[t][mit->first] - mit->second;
			// pointにjump_srcの位置を入れる
			spc.jump_point[t].push_back(mit->second);
		}
	}

	// trackがないならシーケンス終了を置いておく
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
		if(c==0xF0){ // ループ開始
			mul[loop_depth++] = (uint32)seq[p+1] + 1;
		}
		else if(c==0xF9){ // 条件ジャンプ
			mul[loop_depth-1]--;
		}
		else if(c==0xF1){ // ループ終了
			loop_depth--;
		}
		else if(c==0xFA && (p+3)==spc.seq_size[track]){ // トラックループ
			break;
		}
		else if(c==0xF2){ // 終了
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
	// バイナリフォーマット
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

	// SPC作成年月日(16進)
	time_t timer = time(NULL);
	struct tm *local;
	local = localtime(&timer); // 地方時に変換
	header[0x9E] = local->tm_mday; // 日
	header[0x9F] = local->tm_mon+1; // 月
	*(uint16*)(header+0xA0) = local->tm_year+1900; // 年

	// 再生時間(3byte)
	*(uint32*)(header+0xA9) = spc.play_time;
	// フェードアウト時間(4byte)
	*(uint32*)(header+0xAC) = spc.fade_time;
	
	// used to dump
	header[0xD1] = 4; // ETC


	// サウンドメモリ
	uint8 *ram = new uint8[0x10000];
	memset(ram, 0x00, 0x10000);
	ram[0x007D] = 0x01; // プログラム関連
	ram[0x007E] = 0xFF; // 開始タイミング
	ram[0x0092] = 0x80; // 効果音パン
	ram[0x009C] = 0x80; // テンポ
	ram[0x009E] = 0x80; // 効果音テンポ
	ram[0x00A8] = 0x80; // 音程
//	ram[0x00B4] = 0x09; // 効果音エコー
	ram[0x00B8] = 0xFF; // ボリューム関連
	ram[0x00C3] = 0x08; // パン(下位4bit) ok
	ram[0x00C4] = 0x0F; // ボリューム関連(下位4bit) ok
	ram[0x00EB] = 0xFF; // プログラム関連
	ram[0x00F1] = 0x01; // ctrl
	ram[0x00FA] = 0x24; // target0
	ram[0x01FE] = 0x8F; // スタック
	ram[0x01FF] = 0x02; // スタック

	// DSPメモリ
	uint8 dsp_reg[128];
	memset(dsp_reg, 0x00, 128);
	dsp_reg[0x0C] = 0x7F; // MVOL_L
	dsp_reg[0x1C] = spc.f_surround ? 0x81 : 0x7F; // MVOL_R
//	dsp_reg[0x0D] = 0x64; // EFB 設定できない？
	dsp_reg[0x5D] = 0x1B; // DIR
//	dsp_reg[0x6D] = 0xD2; // ESA
//	dsp_reg[0x7D] = 0x05; // EDL
	// エコーバッファ領域設定
	uint16 echobuf_start_adrs = 0xFA00 - (spc.echo_depth << 11);
	dsp_reg[0x6D] = echobuf_start_adrs >> 8; // ESA
	dsp_reg[0x7D] = spc.echo_depth; // EDL


	// ベースアドレス
	// romベースからapu_ramベースに変更しても問題なさそう
	*(uint16*)(ram+0x1C00) = 0x1C14;//0xE5FE;
	
	// ドライバ
	memcpy(ram+0x0200, asd.driver, asd.driver_size);

	// 常駐波形ADSR
	memcpy(ram+0x1A80, asd.sbrr_adsr, 16);
	// 常駐波形音程補正
	memcpy(ram+0x1A00, asd.sbrr_tune, 16);
	// 効果音シーケンス等
	if(spc.f_eseq_out){ // BRRオフセット指定無ければ埋め込む
		memcpy(ram+0x2C00, asd.eseq, asd.eseq_size);
	}

	// 音程補正、ADSR埋め込み
	int i;
	for(i=0; i<spc.brr_map.size(); i++){
		if(spc.brr_map[i].brr_fname.substr(0, 8)=="FF5inst:"){
		//	if(spc.brr_map[i].brr_fname[8]=='s') continue; // 常駐波形はADSR別途
			int sp = 8;
			int ep = term_end(spc.brr_map[i].brr_fname, sp);
			int inst_id = strtol(spc.brr_map[i].brr_fname.substr(sp, ep-sp).c_str(), NULL, 16);
			*(uint16*)(ram+0x1A40+i*2) = asd.brr_tune[inst_id];
			*(uint16*)(ram+0x1AC0+i*2) = asd.brr_adsr[inst_id];
		}
		else{ // 自作BRRでのADSR設定
			*(uint16*)(ram+0x1A40+i*2) = 0x0000; // 音程補正値変えても変化なし？
			ram[0x1AC0+i*2  ] = spc.brr_map[i].adsr1;
			ram[0x1AC0+i*2+1] = spc.brr_map[i].adsr2;
		}
	}

	// 0x1D14からシーケンスデータ
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

	// シーケンスデータを埋め込み
	for(i=0; i<8; i++){
		memcpy(ram + seq_adrs[i], spc.seq[i], spc.seq_size[i]);
	}
//	data[seq_adrs_end] = 0xF2; // 最後は必要ない

	// BRR位置auto
	if(spc.brr_offset==0xFFFF){
		spc.brr_offset = seq_adrs_end;
	}
	seq_adrs_end--;
	printf("SEQ end address 0x%04X\n", seq_adrs_end); //getchar();
	if(seq_adrs_end >= spc.brr_offset){
		printf("Error : #brr_offset 0x%04X がシーケンスと重なっています.\n", spc.brr_offset);
		delete[] ram;
		return -1;
	}
	if(spc.f_eseq_out && seq_adrs_end >= 0x2C00){
		printf("Error : シーケンスが効果音シーケンスと重なっています.\n");
		delete[] ram;
		return -1;
	}

	uint16 seq_rom_adrs_base = *(uint16*)(ram+0x1C00);
//	printf("base_adrs 0x%04X\n", base_adrs);

	// シーケンスROMアドレスの生成
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
	// シーケンスROMアドレスの埋め込み
	for(i=0; i<8; i++){
		*(uint16*)(ram+0x1C02+i*2) = seq_rom_adrs[i];
	}
	*(uint16*)(ram+0x1C12) = seq_rom_adrs_end; // シーケンスアドレスエンド+1を指す

	// ジャンプ制御関連
	{
	int t, i;

	// トラックループアドレス埋め込み
	// FA XX XX
	for(t=0; t<8; t++){
	//	printf("seq%d_size %d\n", t, spc.seq_size[t]);
		if(spc.track_loop[t]!=0xFFFF){ // Lがある場合
			for(i=spc.seq_size[t]-1; i>=0; i--){
				// トラック最後のFAに対してのみループアドレスを調整
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

	// ループブレイクジャンプ埋め込み
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

	// ジャンプアドレス埋め込み
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

	// 常駐波形BRR埋め込み、デフォルトは0x4800
//	memcpy(ram+0x4800, asd.sbrr, asd.sbrr_size);
	// 常駐波形BRR埋め込み
	memcpy(ram+spc.brr_offset, asd.sbrr, asd.sbrr_size);

	// 常駐波形アドレス変更
	uint16 brr_adrs_sa = spc.brr_offset - 0x4800;
	for(i=0; i<8; i++){
		asd.sbrr_start[  i*2] += brr_adrs_sa;
		asd.sbrr_start[1+i*2] += brr_adrs_sa;
	}
	// 常駐波形BRRアドレス埋め込み
	memcpy(ram+0x1B00, asd.sbrr_start, 32);

	// BRR埋め込み
	// すでに埋め込んだBRRは使いまわす
	map<string, pair<uint16, uint16> > brr_put_map;
	uint16 brr_offset = spc.brr_offset + asd.sbrr_size;
	uint32 adrs_index = 0;
	for(i=0; i<spc.brr_map.size(); i++){
		string brr_fname = spc.brr_map[i].brr_fname;
	//	if(brr_fname[0]=='\0') continue;
		uint32 start_adrs, loop_adrs;
		if(brr_put_map.find(brr_fname)==brr_put_map.end()){ // 置いてないbrrなら

			int brr_size;
			uint8 *brr_data;
			if(brr_fname.substr(0, 8)=="FF5inst:"){ // FF5の波形
			//	if(brr_fname[8]=='s') continue; // 常駐波形は飛ばす
				int sp = 8;
				int ep = term_end(brr_fname, sp);
				int inst_id = strtol(brr_fname.substr(sp, ep-sp).c_str(), NULL, 16);
				brr_size = asd.brr_size[inst_id] + 2; // 先頭2バイトループ追加
				brr_data = new uint8[brr_size];
				memcpy(brr_data+2, asd.brr[inst_id], asd.brr_size[inst_id]);
				*(uint16*)brr_data = asd.brr_loop[inst_id];
			//	printf("loop 0x%04X\n",asd.brr_loop[inst_id]);
			}
			else{ // BRRファイル指定
				FILE *brrfp = fopen(brr_fname.c_str(), "rb");
				if(brrfp==NULL){
					printf("Error : BRRファイル %s を開けません.\n", brr_fname.c_str());
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

			// 本家は0x3000は効果音シーケンスで0x4800からBRR
			start_adrs = (uint32)brr_offset + adrs_index;
			loop_adrs = start_adrs + (uint32)(*(uint16*)brr_data);
//printf("%d %s start 0x%X end 0x%X\n", 0x20+i, brr_fname.c_str(), start_adrs, start_adrs+brr_size-2-1);
			if(start_adrs+brr_size-2-1 >= 0x0FA00){
				printf("BRR end address 0x%X\n", start_adrs+brr_size-2-1);
				printf("Error : %s BRRエリアが0xFA00を超えました.\n", brr_fname.c_str());
				delete[] brr_data;
				delete[] ram;
				return -1;
			}
			memcpy(ram+start_adrs, brr_data+2, brr_size-2);
			delete[] brr_data;

			if(start_adrs >= 0x0FA00){
				printf("BRR start address 0x%X\n", start_adrs);
				printf("Error : %s BRRスタートアドレスが0xFA00を超えました.\n", brr_fname.c_str());
				delete[] ram;
				return -1;
			}
			if(loop_adrs >= 0x0FA00){
				printf("BRR loop address 0x%X\n", loop_adrs);
				printf("Error : %s BRRループアドレスが0xFA00を超えました.\n", brr_fname.c_str());
				delete[] ram;
				return -1;
			}

			brr_put_map[brr_fname] = make_pair<uint16, uint16>(start_adrs, loop_adrs);
			adrs_index += brr_size -2;
		}
		else{ // すでに配置したbrrならアドレスを流用するだけ
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
			printf("Error : BRRデータがエコーバッファ領域と重なっています.\n");
			delete[] ram;
			return -1;
		}
	}


	// SPC出力
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
	printf("%s を生成しました.\n\n", spc_fname);
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
				printf("Error : -o %s 出力ファイル名が.spcではありません.\n", output_fname);
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
			printf("不明なオプション？ %s \n", argv[argi]);
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
