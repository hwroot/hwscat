#pragma once

extern const unsigned char sbox[256];
extern const unsigned char inv_sbox[256];
extern const unsigned char hamming_weight[256];

typedef struct _trace_data
{
    unsigned char *points;
    unsigned char point_size;
    double hw_guess;
    double best_abs;
    unsigned int best_pos;
    unsigned char bytes[16];
    unsigned char n_byte;
} trace_data;

using namespace std;

typedef struct _pa_state
{
    int pa_type;            //cpa, dpa
    std::vector<trace_data> traces;
    unsigned int n_smpl_start;
    unsigned int n_points;

    // npoints
    double *p_sum;              //
    double *p_sqr_sum;          // square sum of every trace per each point

    double h_sum[256];
    double h_sqr_sum[256];

    double *h_sum_point;
    double *result;
    std::vector<trace_data *> t0;
    std::vector<trace_data *> t1;
    trace_data dpa_result[256];
} pa_state;

typedef struct _byte_info
{
    double corr;
    unsigned int off;
    unsigned char byte;
} byte_info;

typedef struct _trace_info
{
    double sad;
    unsigned int sad_offset;
    int fd;
    std::string filename;
} trace_info;

extern unsigned char get_hamming_weight(unsigned char guess, unsigned char byte);
extern bool get_lsb(unsigned char guess, unsigned char byte, unsigned char n_bit);
extern double read_trace_point(unsigned int p, trace_data *t);
extern void load_files_for_pa(pa_state *pa);