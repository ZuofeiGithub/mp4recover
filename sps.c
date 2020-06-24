#include <stdio.h>

struct SPS{
    unsigned int profile_idc;
    unsigned int constraint_set0_flag;
    unsigned int constraint_set1_flag;
    unsigned int constraint_set2_flag;
    unsigned int constraint_set3_flag;
    unsigned int constraint_set4_flag;
    unsigned int reserved_zero_3bits;
    unsigned int level_idc;
    unsigned int seq_parameter_set_id;
    unsigned int chroma_format_idc;
    unsigned int separate_colour_plane_flag;
    unsigned int bit_depth_luma_minus8;
    unsigned int bit_depth_chroma_minus8;
    unsigned int qpprime_y_zero_transform_bypass_flag;
    unsigned int seq_scaling_matrix_present_flag;
    unsigned int seq_scaling_list_present_flag[12];
    unsigned int log2_max_frame_num_minus4;
    unsigned int pic_order_cnt_type;
    unsigned int log2_max_pic_order_cnt_lsb_minus4;
    unsigned int delta_pic_order_always_zero_flag;
    int offset_for_non_ref_pic;
    int offset_for_top_to_bottom_field;
    unsigned int num_ref_frames_in_pic_order_cnt_cycle;
    int offset_for_ref_frame[256];
    unsigned int max_num_ref_frames;
    unsigned int gaps_in_frame_num_value_allowed_flag;
    unsigned int pic_width_in_mbs_minus1;
    unsigned int pic_height_in_map_units_minus1;
    unsigned int frame_mbs_only_flag;
	unsigned int mb_adaptive_frame_field_flag;
    unsigned int direct_8x8_inference_flag;
    unsigned int frame_cropping_flag;
	unsigned int frame_crop_left_offset;
	unsigned int frame_crop_right_offset;
	unsigned int frame_crop_top_offset;
	unsigned int frame_crop_bottom_offset;
    unsigned int vui_parameters_present_flag;
	unsigned int aspect_ratio_info_present_flag;
	    unsigned int aspect_ratio_idc;
		unsigned int sar_width;
		unsigned int sar_height;
	unsigned int overscan_info_present_flag;
	    unsigned int overscan_appropriate_flag;
	unsigned int video_signal_type_present_flag;
	    unsigned int video_format;
	    unsigned int video_full_range_flag;
	    unsigned int colour_description_present_flag;
		unsigned int colour_primaries;
	    unsigned int transfer_characteristics;
	    unsigned int matrix_coefficients;
	unsigned int chroma_loc_info_present_flag;
	    unsigned int chroma_sample_loc_type_top_field;
	    unsigned int chroma_sample_loc_type_bottom_field;
	unsigned int timing_info_present_flag;
	    unsigned int num_units_in_tick;
	    unsigned int time_scale;
	    unsigned int fixed_frame_rate_flag;
};

#define ReadBit() _ReadBit(); if(SPSerr){ return; };
#define ReadBits(n) _ReadBits(n); if(SPSerr){ return; };
#define ReadUE() _ReadUE(); if(SPSerr){ return; };
#define ReadSE() _ReadSE(); if(SPSerr){ return; };

unsigned int SPSerr = 0;
unsigned int _SPSlen = 0;
unsigned int CurrentBit = 0;
unsigned char _SPSbuf[512] = {0};
struct SPS SPS;

unsigned int _ReadBit(){
    if(CurrentBit > _SPSlen * 8){ SPSerr = 1; return 0; };
    int index = CurrentBit / 8;
    int offset = CurrentBit % 8 + 1;
    CurrentBit++;
    return (_SPSbuf[index] >> (8 - offset)) & 0x01;
};

unsigned int _ReadBits(unsigned int n){
    int r = 0;
    for(int i = 0; i < n; i++){
	unsigned int rb = _ReadBit();
	if(SPSerr) return 0;
	r |= ( rb << (n-i-1) );
    };
    return r;
};

unsigned int _ReadUE(){
    int r = 0;
    int i = 0;
    while( _ReadBit() == 0 && i < 32 && !SPSerr ) i++;
    if(SPSerr) return 0;
    r = _ReadBits(i);
    if(SPSerr) return 0;
    r += (1 << i) - 1;
    return r;
};

int _ReadSE(){
    int r = _ReadUE();
    if(SPSerr)return 0;
    if(r & 0x01) r = (r + 1) / 2; else r = -(r / 2);
    return r;
};

void DecodeSPS(unsigned char *SPSbuf, int SPSlen, unsigned int *frame_width, unsigned int *frame_height, unsigned int *frame_rate){
    _SPSlen=0;
    for(int i = 0; i < SPSlen; i++){
	if( i+2 < SPSlen && ( SPSbuf[i] == 0 && SPSbuf[i + 1] == 0 && SPSbuf[i + 2] == 3 ) ){ // correct for emul symbols 00 00 03 xx
	    _SPSbuf[_SPSlen++] = SPSbuf[i];
	    _SPSbuf[_SPSlen++] = SPSbuf[i + 1];
	    i += 2;
	}else{
	    _SPSbuf[_SPSlen++] = SPSbuf[i];
	};
    };
    
    SPSerr = 0;
    CurrentBit = 0;
    SPS.profile_idc = ReadBits(8);
    SPS.constraint_set0_flag = ReadBit();
    SPS.constraint_set1_flag = ReadBit();
    SPS.constraint_set2_flag = ReadBit();
    SPS.constraint_set3_flag = ReadBit();
    SPS.constraint_set4_flag = ReadBit();
    SPS.reserved_zero_3bits = ReadBits(3);
    SPS.level_idc = ReadBits(8);
    SPS.seq_parameter_set_id = ReadUE();
    if(SPS.profile_idc == 100 || SPS.profile_idc == 110 || SPS.profile_idc == 122 || SPS.profile_idc == 244 || SPS.profile_idc == 44 || SPS.profile_idc == 83 || SPS.profile_idc == 86 || SPS.profile_idc == 118 || SPS.profile_idc == 0x43){
	SPS.chroma_format_idc = ReadUE();
	if(SPS.chroma_format_idc == 3){
	    SPS.separate_colour_plane_flag = ReadBit();
	};
	SPS.bit_depth_luma_minus8 = ReadUE();
	SPS.bit_depth_chroma_minus8 = ReadUE();
	SPS.qpprime_y_zero_transform_bypass_flag = ReadBit();
	SPS.seq_scaling_matrix_present_flag = ReadBit();
	if(SPS.seq_scaling_matrix_present_flag){;};
    };
	
    SPS.log2_max_frame_num_minus4 = ReadUE();
    SPS.pic_order_cnt_type = ReadUE();
    if(SPS.pic_order_cnt_type == 0){
        SPS.log2_max_pic_order_cnt_lsb_minus4 = ReadUE();
    }else if(SPS.pic_order_cnt_type == 1){
        SPS.delta_pic_order_always_zero_flag = ReadBit();
        SPS.offset_for_non_ref_pic = ReadSE();
        SPS.offset_for_top_to_bottom_field = ReadSE();
        SPS.num_ref_frames_in_pic_order_cnt_cycle = ReadUE();
    };
    SPS.max_num_ref_frames = ReadUE();
    SPS.gaps_in_frame_num_value_allowed_flag = ReadBit();
    SPS.pic_width_in_mbs_minus1 = ReadUE();
    SPS.pic_height_in_map_units_minus1 = ReadUE();
    SPS.frame_mbs_only_flag = ReadBit();
    if(!SPS.frame_mbs_only_flag){
	SPS.mb_adaptive_frame_field_flag = ReadBit();
    };
    SPS.direct_8x8_inference_flag = ReadBit();
    SPS.frame_cropping_flag = ReadBit();
    if(SPS.frame_cropping_flag){
	SPS.frame_crop_left_offset = ReadUE();
	SPS.frame_crop_right_offset = ReadUE();
	SPS.frame_crop_top_offset = ReadUE();
	SPS.frame_crop_bottom_offset = ReadUE();
    };
    *frame_width = (SPS.pic_width_in_mbs_minus1 + 1) * 16 - SPS.frame_crop_left_offset * 2 - SPS.frame_crop_right_offset * 2;
    *frame_height = (2 - SPS.frame_mbs_only_flag) * (SPS.pic_height_in_map_units_minus1 + 1) * 16 - SPS.frame_crop_top_offset * 2 - SPS.frame_crop_bottom_offset * 2;
    
    SPS.vui_parameters_present_flag = ReadBit();
    if(SPS.vui_parameters_present_flag){
	SPS.aspect_ratio_info_present_flag = ReadBit();
        if(SPS.aspect_ratio_info_present_flag){;};
        SPS.overscan_info_present_flag = ReadBit();
        if(SPS.overscan_info_present_flag){;};
        SPS.video_signal_type_present_flag = ReadBit();
        if(SPS.video_signal_type_present_flag){
    	    SPS.video_format = ReadBits(3);
	    SPS.video_full_range_flag = ReadBit();
	    SPS.colour_description_present_flag = ReadBit();
	    if(SPS.colour_description_present_flag){
		SPS.colour_primaries = ReadBits(8);
		SPS.transfer_characteristics = ReadBits(8);
		SPS.matrix_coefficients = ReadBits(8);
	    };
	};
        SPS.chroma_loc_info_present_flag = ReadBit();
        if(SPS.chroma_loc_info_present_flag){
	    SPS.chroma_sample_loc_type_top_field = ReadUE();
	    SPS.chroma_sample_loc_type_bottom_field = ReadUE();
        };
        SPS.timing_info_present_flag = ReadBit();
	if(SPS.timing_info_present_flag){
	    SPS.num_units_in_tick = ReadBits(32);
	    SPS.time_scale = ReadBits(32);
	    SPS.fixed_frame_rate_flag = ReadBit();
	    *frame_rate = SPS.time_scale / SPS.num_units_in_tick / 2;
	};
    };
};
