#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <netinet/in.h>

#define d_hdlrlen 33
#define sttslen 24
#define vmhdlen 20
#define dinflen 36
#define mdhdlen 32
#define hdlrlen 45
#define tkhdlen 92
#define edtslen 36
#define mvhdlen 108

void DecodeSPS(unsigned char *SPSbuf, int SPSlen, unsigned int *frame_width, unsigned int *frame_height, unsigned int *frame_rate); // sps.c

int main(int arg_cnt, char *args[]){
    FILE *in_file;
    FILE *out_file;
    unsigned char buf[1024 * 1024 * 16];
    unsigned int atom_length;
    unsigned int FrameRate = 0;
    unsigned char out_dir[512] = {0};
    
    for(int arg_num = 1; arg_num < arg_cnt; arg_num++){
	unsigned char SPS[128] = {0};
	unsigned short SPS_length = 0;
	unsigned char PPS[64] = {0};
	unsigned short PPS_length = 0;
	unsigned short SEI_length = 0;
	unsigned int samples[1024 * 1024] = {0};
	unsigned int *sam_p = samples;
	unsigned int sizes[1024 * 1024] = {0};
	unsigned int *siz_p = sizes;
	unsigned int offsets[1024 * 1024] = {0};
	unsigned int *off_p = offsets;
	unsigned int Frames = 0;
	unsigned int KeyFrames = 0;
	unsigned int SampleNum = 1;
	unsigned int data_offset = 0;
	unsigned int out_offset = 0;
	struct stat stat_struct;
	unsigned int FrameWidth = 0;
	unsigned int FrameHeight = 0;
	
	char file_name[512] = {0};
	strcpy(file_name, args[arg_num]);
	
	if( strncmp(file_name, args[0] + 2, strlen(file_name)) == 0 ) continue;
		
	if( strncmp(file_name, "-r", 2) == 0 && strlen(file_name) == 2 ){
	    FrameRate = atoi(args[++arg_num]);
	    continue;
	};
	
	if( strncmp(file_name, "-o", 2) == 0 ){
	    strcpy((char *)out_dir, args[++arg_num]);
	    mkdir((char *)out_dir, 755);
	    continue;
	};
	
	printf("try file: %s\n", file_name);
	if( (in_file = fopen(file_name, "rb")) == NULL ){
	    printf("fopen() error: %s\n", file_name);
	    continue;
	};
	stat(file_name, &stat_struct);
	if(S_ISDIR(stat_struct.st_mode)) continue;
	
	sprintf(file_name, "%s.out", file_name);
	if( (out_file = fopen(file_name, "wb")) == NULL ){
	    printf("fopen() error: %s\n", file_name);
	    continue;
	};

// read corrupt file
	while(1){
	    int rlen = fread(buf, 1, 8, in_file);
	    if(rlen == 8){
		memcpy(&atom_length, buf, 4);
		atom_length = htonl(atom_length);
	    
		if( strncmp((char *)buf + 4, "ftyp", 4) == 0 ){
		    rlen = fread(buf + 8, 1, atom_length - 8, in_file);
		    if(rlen == atom_length - 8){
			fwrite(buf, 1, atom_length, out_file);
			data_offset += atom_length;
			continue;
		    };
		};

		if( strncmp((char *)buf + 4, "free", 4) == 0 ){
		    fwrite(buf, 1, atom_length, out_file);
		    data_offset += atom_length;
		    continue;
		};
		
		if( strncmp((char *)buf + 4, "mdat", 4) == 0 ){
		    fwrite(buf, 1, 8, out_file);
		    data_offset += 8;
		    continue;
		};
	    
		unsigned char nal_type = buf[4] & 0x1F;
		if(!(nal_type == 1 || nal_type == 5 || nal_type == 6 || nal_type == 7 || nal_type == 8) || (atom_length > sizeof(buf) - 8)) break;
		if(atom_length > 4) rlen = fread(buf + 8, 1, atom_length - 4, in_file);
		
		if(rlen == atom_length - 4 || atom_length == 4){
		    if(nal_type == 7){
			if(SPS_length == 0){
			    SPS_length = (short)atom_length;
			    memcpy(SPS, buf + 4, SPS_length);
			};
			*off_p++ = htonl(data_offset + out_offset);
		    };
		    if(nal_type == 8){
			if(PPS_length == 0){
			    PPS_length = (short)atom_length;
			    memcpy(PPS, buf + 4, PPS_length);
			};
		    };
		    if(nal_type == 6){
			if(SEI_length == 0){
			    SEI_length = (short)atom_length;
			};
		    };
		    if(nal_type == 5){
			*sam_p++ = SampleNum++ | 0x80000000;
			*siz_p++ = htonl(4 + SPS_length + 4 + PPS_length + 4 + SEI_length + 4 + atom_length);
			KeyFrames++;
			Frames++;
		    }
		    if(nal_type == 1){
			*sam_p++ = SampleNum++;
			*siz_p++ = htonl(4 + atom_length);
			Frames++;
		    };
		    
		    fwrite(buf, 1, 4 + atom_length, out_file);
		    out_offset += 4 + atom_length;
		    
		}else{
		    break;
		};
		continue;
	    }else{
	    };
	    break;
	};
	
	fclose(in_file);

// forming moov atom
	unsigned char encoder[] = "MP4RECOVER";
	unsigned int avcClen = 8 + 6 + 2 + SPS_length + 1 + 2 + PPS_length + 1, _avcClen = htonl(avcClen);
	unsigned int avc1len = avcClen + 86, _avc1len = htonl(avc1len);
	unsigned int stsdlen = avc1len + 16, _stsdlen = htonl(stsdlen);
	unsigned int stsslen = 16 + KeyFrames * 4, _stsslen = htonl(stsslen);
	unsigned int stsclen = 16 + KeyFrames * 12, _stsclen = htonl(stsclen);
	unsigned int stszlen = 20 + Frames * 4, _stszlen = htonl(stszlen);
	unsigned int stcolen = 16 + KeyFrames * 4, _stcolen = htonl(stcolen);
	unsigned int datalen = 16 + sizeof(encoder) - 1, _datalen = htonl(datalen);
	unsigned int ilstlen = 16 + datalen, _ilstlen = htonl(ilstlen), _toolen = htonl(ilstlen - 8);
	unsigned int udtalen = 20 + d_hdlrlen + ilstlen, _udtalen = htonl(udtalen), _metalen = htonl(udtalen - 8);
	unsigned int stbllen = 8 + stsdlen + sttslen + stsslen + stsclen + stszlen + stcolen, _stbllen = htonl(stbllen);
	unsigned int minflen = 8 + vmhdlen + dinflen + stbllen, _minflen = htonl(minflen);
	unsigned int mdialen = 8 + mdhdlen + hdlrlen + minflen, _mdialen = htonl(mdialen);
	unsigned int traklen = 8 + tkhdlen + edtslen + mdialen, _traklen = htonl(traklen);
	unsigned int moovlen = 8 + mvhdlen + traklen + udtalen, _moovlen = htonl(moovlen);

	DecodeSPS(SPS, SPS_length, &FrameWidth, &FrameHeight, &FrameRate);
	if(FrameRate == 0) FrameRate = 25;
	unsigned int Duration = htonl(((double)Frames / FrameRate) * 1000 + 1);
	
	unsigned char mvhd[mvhdlen] = {0}; // Movie Header Atom
	mvhd[3] = 0x6C; // mvhd atom length
	strncpy((char *)mvhd + 4, "mvhd", 4);
	unsigned int TimeScale = htonl(0x3E8); // 1000
	memcpy(mvhd + 20, &TimeScale, 4); // time scale (1000 => time unit = 1/1000 s)
	memcpy(mvhd + 24, &Duration, 4); // duration in time units
	mvhd[29] = 0x01; // preffered rate
	mvhd[32] = 0x01; // preffered volume
	mvhd[45] = 0x01; // matrix
	mvhd[61] = 0x01; // matrix
	mvhd[76] = 0x40; // matrix
	mvhd[107] = 0x02; // next track ID
	
	unsigned char tkhd[tkhdlen] = {0}; // Movie Track Header Atom
	tkhd[3] = 0x5C; // tkhd atom length
	strncpy((char *)tkhd + 4, "tkhd", 4);
	tkhd[11] = 0x03; // flags
	tkhd[23] = 0x01; // track ID
	memcpy(tkhd + 28, &Duration, 4); // duration in time units
	memcpy(tkhd + 48, mvhd + 44, 36); // matrix

	unsigned short width = htons(FrameWidth);
	unsigned short height = htons(FrameWidth);
	memcpy(tkhd + 84, &width, 2); // Track Width
	memcpy(tkhd + 88, &height, 2); // Track Height
	
	unsigned char edts[edtslen] = {0}; // Edit Atom & Edit List Atom
	edts[3] = 0x24; // edts atom length
	strncpy((char *)edts + 4, "edts", 4);
	edts[11] = 0x1C; // elst atom length
	strncpy((char *)edts + 12, "elst", 4);
	edts[23] = 0x01; // number of edit list entries
	memcpy(edts + 24, &Duration, 4); // duration in time units
	edts[33] = 0x01; // playback speed

	unsigned char mdhd[mdhdlen] = {0}; // Media Header Atom
	mdhd[3] = 0x20; // mdhd atom length
	strncpy((char *)mdhd + 4, "mdhd", 4);
	unsigned int TimeBase = 90000;
	unsigned int _TimeBase = htonl(TimeBase);
	memcpy(mdhd + 20, &_TimeBase, 4); // time base = 90000
	unsigned int _Duration = htonl((double)90000 / FrameRate * Frames);
	memcpy(mdhd + 24, &_Duration, 4); // duration in time units
	mdhd[28] = 0x55; // language
	mdhd[29] = 0xC4; // language

	unsigned char hdlr[hdlrlen] = {0}; // Media Header Reference Atom
	hdlr[3] = 0x2D; // hdlr atom length
	strncpy((char *)hdlr + 4, "hdlr", 4);
	strncpy((char *)hdlr + 16, "vide", 4);
	strncpy((char *)hdlr + 32, "VideoHandler", 12);
	
	unsigned char vmhd[vmhdlen] = {0}; // Video Media Information Header Atom
	vmhd[3] = 0x14; // vmhd atom length
	strncpy((char *)vmhd + 4, "vmhd", 4);
	vmhd[11] = 0x01; // media information flags
	
	unsigned char dinf[dinflen] = {0}; // Video Media Data Information Atom & Media Data Reference Atom & url Atom
	dinf[3] = 0x24; // dinf atom length
	strncpy((char *)dinf + 4, "dinf", 4);
	dinf[11] = 0x1C; // dref atom length
	strncpy((char *)dinf + 12, "dref", 4);
	dinf[23] = 0x01; // number of entries
	dinf[27] = 0x0C; // url atom length
	strncpy((char *)dinf + 28, "url ", 4);
	dinf[35] = 0x01; // data flags

	unsigned char *avcC = (unsigned char *)malloc(avcClen);
	memset(avcC, 0, avcClen);
	memcpy(avcC, &_avcClen, 4);
	strncpy((char *)avcC + 4, "avcC", 4);
	avcC[8] = 0x01;
	memcpy(avcC + 9, SPS + 1, 3);
	avcC[12] = 0xFF;
	avcC[13] = 0xE1;
	unsigned short _SPS_length = htons(SPS_length);
	memcpy(avcC + 14, &_SPS_length, 2);
	memcpy(avcC + 16, SPS, SPS_length);
	avcC[16 + SPS_length] = 0x01;
	unsigned short _PPS_length = htons(PPS_length + 1);
	memcpy(avcC + 16 + SPS_length + 1, &_PPS_length, 2);
	memcpy(avcC + 16 + SPS_length + 1 + 2, PPS, PPS_length);

	unsigned char *avc1 = (unsigned char *)malloc(avc1len);
	memset(avc1, 0, avc1len);
	memcpy(avc1, &_avc1len, 4);
	strncpy((char *)avc1 + 4, "avc1", 4);
	avc1[15] = 0x01; // data referebce index
	memcpy(avc1 + 32, &width, 2); // video width
	memcpy(avc1 + 34, &height, 2); // video height
	avc1[37] = 0x48; // horizontal video resolution, 72 pix/inch
	avc1[41] = 0x48; // vertical video resolution, 72 pix/inch
	avc1[49] = 0x01; // frame count
	avc1[83] = 0x18; // color depth
	avc1[84] = 0xFF; // -1
	avc1[85] = 0xFF; // -1
	memcpy(avc1 + 86, avcC, avcClen);

	unsigned char *stsd = (unsigned char *)malloc(stsdlen); // Sample Description Atom
	memset(stsd, 0, stsdlen);
	memset(stsd, 0, stsdlen);
	memcpy(stsd, &_stsdlen, 4);
	strncpy((char *)stsd + 4, "stsd", 4);
	stsd[15] = 0x01; // number of entries
	memcpy(stsd + 16, avc1, avc1len);

	unsigned char stts[sttslen] = {0}; // Sample To Time Table Atom
	stts[3] = 0x18; // stts atom length
	strncpy((char *)stts + 4, "stts", 4);
	stts[15] = 0x01; // number of entries
	_Duration = htonl(Frames);
	memcpy(stts + 16, &_Duration, 4); // frame count
	unsigned int FrameDuration = htonl(TimeBase / FrameRate);
	memcpy(stts + 20, &FrameDuration, 4); // frame duration (TimeBase / FrameRate)

	unsigned char *stss = (unsigned char *)malloc(stsslen); // Sample Table Sync Sample Atom
	memset(stss, 0, stsslen);
	memcpy(stss, &_stsslen, 4); // stss atom length
	strncpy((char *)stss + 4, "stss", 4);
	unsigned int _KeyFrames = htonl(KeyFrames);
	memcpy(stss + 12, &_KeyFrames, 4); // number of sync sample table entries
	unsigned int KeyFrameIndex = 0;
	unsigned int KeyFrameNum = 1;
	unsigned int _KeyFrameIndex = 0;
	for(int i = 0; i < SampleNum - 1; i++){
	    if( (*(samples + i) & 0x80000000) > 0 ){
		_KeyFrameIndex = htonl(*(samples + i) & 0x7FFFFFFF);
		memcpy(stss + 16 + KeyFrameIndex++ * 4, &_KeyFrameIndex, 4);
	    };
	};

	unsigned char *stsc = (unsigned char *)malloc(stsclen); // Sample To Chunk Atom
	memset(stsc, 0, stsclen);
	memcpy(stsc, &_stsclen, 4); // stsc atom length
	strncpy((char *)stsc + 4, "stsc", 4);
	memcpy(stsc + 12, &_KeyFrames, 4); // number of entries
	KeyFrameIndex = 0;
	_KeyFrameIndex = 1;
	unsigned int ChunkNum = 0;
	for(int i = 0; i < SampleNum - 1; i++){
	    if( (*(samples + i) & 0x80000000) > 0 ){
		unsigned int _ChunkNum = htonl(ChunkNum + 1);
		unsigned int sdID = htonl(1);
		int j = 0;
		for(j = i + 1; j < SampleNum - 1; j++){
		    if( (*(samples + j) & 0x80000000) > 0 ){
			break;
		    };
		};
		unsigned int SamplesPerChunk = htonl(j - i);
		memcpy(stsc + 16 + ChunkNum * 12, &_ChunkNum, 4);
		memcpy(stsc + 16 + ChunkNum * 12 + 4, &SamplesPerChunk, 4);
		memcpy(stsc + 16 + ChunkNum * 12 + 8, &sdID, 4);
		ChunkNum++;
	    };
	};

	unsigned char *stsz = (unsigned char *)malloc(stszlen); // Sample Size Box Atom
	memset(stsz, 0, stszlen);
	memcpy(stsz, &_stszlen, 4); // stsz atom length
	strncpy((char *)stsz + 4, "stsz", 4);
	unsigned int _Frames = htonl(Frames);
	memcpy(stsz + 16, &_Frames, 4); // number of entries
	memcpy(stsz + 20, sizes, Frames * 4);

	unsigned char *stco = (unsigned char *)malloc(stcolen); // Chunk Offset Box Atom
	memset(stco, 0, stcolen);
	memcpy(stco, &_stcolen, 4); // stco atom length
	strncpy((char *)stco + 4, "stco", 4);
	memcpy(stco + 12, &_KeyFrames, 4); // number of entries
	off_p = offsets;
	memcpy(stco + 16, offsets, KeyFrames * 4);

	unsigned char d_hdlr[d_hdlrlen] = {0}; // hdlr atom
	d_hdlr[3] = 0x21; // hdlr atom length
	strncpy((char *)d_hdlr + 4, "hdlr", 4);
	strncpy((char *)d_hdlr + 16, "mdirappl", 8);

	unsigned char *data = (unsigned char *)malloc(datalen); // User Data Atom
	memset(data, 0, datalen);
	memcpy(data, &_datalen, 4); // data atom length
	strncpy((char *)data + 4, "data", 4);
	data[11] = 1;
	memcpy(data + 16, encoder, sizeof(encoder) - 1);

	unsigned char *ilst = (unsigned char *)malloc(ilstlen); // ilst Atom
	memset(ilst, 0, ilstlen);
	memcpy(ilst, &_ilstlen, 4); // ilst atom length
	strncpy((char *)ilst + 4, "ilst", 4);
	memcpy(ilst + 8, &_toolen, 4); // too atom length
	ilst[12] = 0xA9;
	strncpy((char *)ilst + 13, "too", 3);
	memcpy(ilst + 16, data, datalen);

	unsigned char *udta = (unsigned char *)malloc(udtalen); // User Data Atom
	memset(udta, 0, udtalen);
	memcpy(udta, &_udtalen, 4); // udta atom length
	strncpy((char *)udta + 4, "udta", 4);
	memcpy(udta + 8, &_metalen, 4); // meta atom length
	strncpy((char *)udta + 12, "meta", 4);
	memcpy(udta + 20, d_hdlr, sizeof(d_hdlr));
	memcpy(udta + 20 + sizeof(d_hdlr), ilst, ilstlen);
    
	unsigned char *stblhdr = (unsigned char *)malloc(8); // Sample Table Atom
	memcpy(stblhdr, &_stbllen, 4); // stbl atom length
	strncpy((char *)stblhdr + 4, "stbl", 4);

	unsigned char *minfhdr = (unsigned char *)malloc(8); // Media Information Atom
	memcpy(minfhdr, &_minflen, 4); // minf atom length
	strncpy((char *)minfhdr + 4, "minf", 4);

	unsigned char *mdiahdr = (unsigned char *)malloc(8); // Media Atom
	memcpy(mdiahdr, &_mdialen, 4); // mdia atom length
	strncpy((char *)mdiahdr + 4, "mdia", 4);

	unsigned char *trakhdr = (unsigned char *)malloc(8); // Track Atom
	memcpy(trakhdr, &_traklen, 4); // trak atom length
	strncpy((char *)trakhdr + 4, "trak", 4);

	unsigned char *moovhdr = (unsigned char *)malloc(8); // Movie Atom
	memcpy(moovhdr, &_moovlen, 4); // moov atom length
	strncpy((char *)moovhdr + 4, "moov", 4);

	fwrite(moovhdr, 1, 8, out_file);
	fwrite(mvhd, 1, sizeof(mvhd), out_file);
	fwrite(trakhdr, 1, 8, out_file);
	fwrite(tkhd, 1, sizeof(tkhd), out_file);
	fwrite(edts, 1, sizeof(edts), out_file);
	fwrite(mdiahdr, 1, 8, out_file);
	fwrite(mdhd, 1, sizeof(mdhd), out_file);
	fwrite(hdlr, 1, sizeof(hdlr), out_file);
	fwrite(minfhdr, 1, 8, out_file);
	fwrite(vmhd, 1, sizeof(vmhd), out_file);
	fwrite(dinf, 1, sizeof(dinf), out_file);
	fwrite(stblhdr, 1, 8, out_file);
	fwrite(stsd, 1, stsdlen, out_file);
	fwrite(stts, 1, sizeof(stts), out_file);
	fwrite(stss, 1, stsslen, out_file);
	fwrite(stsc, 1, stsclen, out_file);
	fwrite(stsz, 1, stszlen, out_file);
	fwrite(stco, 1, stcolen, out_file);
	fwrite(udta, 1, udtalen, out_file);

	fseek(out_file, data_offset - 8, SEEK_SET);
	out_offset = htonl(out_offset + 8);
	fwrite(&out_offset, sizeof(unsigned int), 1, out_file); // write correct mdat atom length
	
	fclose(out_file);

// rename output file
	struct tm tm_struct = *localtime(&stat_struct.st_birthtime);
	char start[64] = {0};
	strftime(start, sizeof(start), "%Y-%m-%d__%H-%M-%S", &tm_struct);
	stat_struct.st_birthtime += htonl(Duration) / 1000;
	tm_struct = *localtime(&stat_struct.st_birthtime);
	char stop[24] = {0};
	strftime(stop, sizeof(start), "%H-%M-%S", &tm_struct);
	sprintf(start, "%s--%s.mp4", start, stop);

	char *slash = strrchr(file_name, '/');
	if(strlen((char *)out_dir) != 0){
	    char old_file_name[512] = {0};
	    strcpy(old_file_name, file_name);
	    sprintf(file_name, "%s/%s%c", out_dir, start, 0);
	    rename(old_file_name, file_name);
	}else if(slash != NULL){
	    char old_file_name[512] = {0};
	    strcpy(old_file_name, file_name);
	    *slash = 0;
	    sprintf(file_name, "%s/%s%c", file_name, start, 0);
	    rename(old_file_name, file_name);
	}else{
	    rename(file_name, start);
	};
    };
};
