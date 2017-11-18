#include "stdafx.h"



void init_pa_state(pa_state *pa, int pa_type )
{
    pa->p_sum = NULL;
    pa->p_sqr_sum = NULL;
    pa->result = NULL;
    pa->h_sum_point = NULL;

    if (pa_type == 0)
    {
        pa->p_sum = (double *)calloc(pa->n_points, sizeof(double));
        pa->p_sqr_sum = (double *)calloc(pa->n_points, sizeof(double));

        pa->result = (double *)calloc(pa->n_points * sizeof(double), 256);
        pa->h_sum_point = (double *)calloc(pa->n_points * sizeof(double), 256);


        memset(pa->h_sum, 0, sizeof(pa->h_sum));
        memset(pa->h_sqr_sum, 0, sizeof(pa->h_sqr_sum));
    }
    else if (pa_type == 1)
    {        
        pa->pa_type = pa_type;
        memset(pa->dpa_result, 0, sizeof(pa->dpa_result));
    }
}

void free_pa_state(pa_state *pa, bool keep_points )
{
    if(pa->p_sum)
        free(pa->p_sum);
    
    if(pa->p_sqr_sum)
        free(pa->p_sqr_sum);
    
    if(pa->result)
        free(pa->result);
    
    if( pa->h_sum_point )
        free(pa->h_sum_point);

    if (pa->pa_type == 1)
    {
        for (unsigned int i = 0; i < _countof(pa->dpa_result); i++)
        {
            if (pa->dpa_result[i].points)
            {
                free(pa->dpa_result[i].points);
            }
        }
    }

    if (keep_points)
        return;

    std::vector<trace_data>::iterator tdB = pa->traces.begin();
    std::vector<trace_data>::iterator tdE = pa->traces.end();

    while (tdB != tdE)
    {
        trace_data &td = *tdB;
        if (td.points)
        {
            free(td.points);
        }
        tdB++;
    }

    pa->traces.clear();
}

//unsigned char model(unsigned char guess, unsigned char byte)
//{
//    isbox[  ]
//}

void cpa_calculate_sum(pa_state *pa)
{
    size_t ntraces = pa->traces.size();
    unsigned int npoints = pa->n_points;

    for (size_t i = 0; i < ntraces; i++)
    {
        trace_data &t = pa->traces[i];

        for (unsigned int p = 0; p < npoints; p++)
        {
            pa->p_sqr_sum[p] += read_trace_point(p, &t) * read_trace_point(p, &t);
            pa->p_sum[p] += read_trace_point(p, &t);
        }        
    }

}

void cpa_process_byte(unsigned char byte_index, pa_state *pa)
{
    double sum1, sum2, sum3;
    unsigned int ntraces = (unsigned int)pa->traces.size();
    unsigned int npoints = pa->n_points;   
    
    for (unsigned int guess_byte = 0; guess_byte < 256; guess_byte++)
    {
        //printf("\rguess byte: %u", guess_byte);

        for (unsigned int i = 0; i < ntraces; i++)
        {
            trace_data &t = pa->traces[i];
            
            unsigned char hw = get_hamming_weight(guess_byte, t.bytes[byte_index]);

            t.hw_guess = (double)hw;
            pa->h_sum[guess_byte] += (double)hw;
            pa->h_sqr_sum[guess_byte] += (double)hw * (double)hw;
        }

        for (unsigned int i = 0; i < ntraces; i++)
        {
            trace_data &t = pa->traces[i];
            
            for (unsigned int p = 0; p < npoints; p++)
                pa->h_sum_point[guess_byte * npoints + p] += read_trace_point(p, &t) * t.hw_guess;
        }

        sum1 = (pa->h_sum[guess_byte] * pa->h_sum[guess_byte]) - (double)ntraces * pa->h_sqr_sum[guess_byte];

        for (unsigned int p = 0; p < npoints; p++)
        {
            sum2 = (pa->p_sum[p] * pa->p_sum[p]) - (double)ntraces * pa->p_sqr_sum[p];
            sum3 = (double)ntraces * pa->h_sum_point[guess_byte * npoints + p] - pa->h_sum[guess_byte] * pa->p_sum[p];
            pa->result[npoints * guess_byte + p] = sum3 / sqrt(sum1 * sum2);
        }
    }    
}

int load_trace_file(std::string filepath, pa_state *pa )
{
    int fd, err;
    long pos;
    trace_data td;
    std::string dic = "0123456789abcdef";    

    td.point_size = 8;

    err = _sopen_s(&fd, filepath.c_str(), _O_BINARY | _O_RDONLY, _SH_DENYWR, _S_IREAD);
    
    if (0 != err)
        return err;    
    
    if (-1 == ( pos = _lseek(fd, (long)pa->n_smpl_start * td.point_size, SEEK_SET)))
    {
        err = errno;
        goto _ret_err;
    }    
    
    size_t p = filepath.rfind( '\\' );
    unsigned nread, nbytes = pa->n_points * td.point_size;

    const char *fname = filepath.c_str() + p + 1;
    
    for (int i = 0; i < 32; i += 2 )
    {
        unsigned char byte;
        char cBytes;

        cBytes = fname[i];

        size_t npos1 = dic.find(fname[i]);
        size_t npos2 = dic.find(fname[i+1]);

        byte = (unsigned char)((npos1 << 4 ) | npos2);

        td.bytes[i / 2] = byte;
    }    
    
    td.hw_guess = 0;
    td.points = (unsigned char *)malloc(nbytes);
    
    if (NULL == td.points)
    {
        err = ENOMEM;
        goto _ret_err;
    }
    
    nread = _read(fd, td.points, nbytes);

    if (nread != nbytes)
    {
        err = EIO;
        goto _ret_err;
    }
    
    _close(fd);

    pa->traces.push_back(td);

    err = 0;

_ret_err:

    if (0 != err)
    {
        if (NULL != td.points)
            free((void *)td.points);

        if( fd > 0 )
            _close(fd);
    }

    return err;
}

