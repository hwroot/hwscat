#include "stdafx.h"

double *get_trace_avg(std::vector<trace_data *> &t, unsigned int npoints )
{    
    double *avg = (double *)calloc(npoints, sizeof(double));
    
    double tsize = (double)t.size();
    std::vector<trace_data *>::iterator itB = t.begin();
    std::vector<trace_data *>::iterator itE = t.end();

    while (itB != itE)
    {
        trace_data *td = *itB;
        
        for (unsigned int n = 0; n < npoints; n++)
        {
            double pavg = read_trace_point(n, td) / tsize;
            avg[n] += pavg;
        }
             //(double)( (double)(*(short *)&td->points[n*2])) / tsize;

        itB++;
    }

    return avg;
}

void dpa_process_traces(unsigned char byte_index, pa_state *pa, unsigned char n_bit)
{
    unsigned int ntraces = (unsigned int)pa->traces.size();
    unsigned int npoints = pa->n_points;

    for (unsigned int guess_byte = 0; guess_byte < 256; guess_byte++)
    {
        pa->t0.clear();
        pa->t1.clear();

        for (unsigned int i = 0; i < ntraces; i++)
        {
            trace_data &t = pa->traces[i];

            bool bit = get_lsb(guess_byte, t.bytes[byte_index], n_bit );

            if (bit)
            {
                pa->t1.push_back(&t);
            }
            else
            {
                pa->t0.push_back(&t);
            }
        }

        double *t0Avg = get_trace_avg(pa->t0, npoints);
        double *t1Avg = get_trace_avg(pa->t1, npoints);

        trace_data *td = &pa->dpa_result[guess_byte];
        td->best_abs = 0;
        //td->points = (unsigned char *)malloc(sizeof(double) * npoints);
        td->points = NULL;
        int decim = 1;
       //+ t1Avg[n + 2] + t1Avg[n + 3] + t1Avg[n + 4] + t1Avg[n + 5]
       //+ t0Avg[n + 2] + t0Avg[n + 3] + t0Avg[n + 4] + t0Avg[n + 5]
        for (unsigned int n = 0; n < npoints / decim - decim; n += decim)
        {
            double p1 = (t1Avg[n] ) / decim;
            double p0 = (t0Avg[n] ) / decim;
            
            //double a = abs(t1Avg[n] - t0Avg[n]);
            double a = abs(p1 - p0);
            //*(double *)&td->points[n * sizeof(double)] = a;

            if (a > td->best_abs)
            {
                td->best_abs = a;
                td->best_pos = n;
            }
        }

        free(t0Avg);
        free(t1Avg);
    }
}

int qsort_cmp_trace_data_by_dpa_best(const void *a1, const void *a2)
{
    const trace_data *td1 = (const trace_data *)(a1);
    const trace_data *td2 = (const trace_data *)(a2);

    if (td1->best_abs > td2->best_abs)
        return -1;
    else if (td1->best_abs < td2->best_abs)
        return 1;

    return 0;
}

void dpa_sweep(pa_state &pa, unsigned int n_byte)
{
    trace_data dpa_best[256];
    unsigned int pos_best[256 * 8];

    memset(dpa_best, 0, sizeof(dpa_best));
    memset(pos_best, 0, sizeof(pos_best));

    for (unsigned char n_bit = 0; n_bit < 8; n_bit++)
    {
        //printf("dpa bit: %u\r\n", n_bit);
        init_pa_state(&pa, 1);
        load_files_for_pa(&pa);
        dpa_process_traces(n_byte, &pa, n_bit);

        //memcpy(&dpa_best[n_bit * 256], pa.dpa_result, sizeof(pa.dpa_result));

        for (unsigned int n = 0; n < 256; n++) 
        {
            trace_data *td = &pa.dpa_result[n];
            dpa_best[n].best_abs += td->best_abs;
            dpa_best[n].n_byte = n;
            pos_best[(n_bit * 256) + n] = td->best_pos;
            //printf("%02x %f %u\r\n", n, td->best_abs, td->best_pos);
        }

        free_pa_state(&pa, sad_array.empty() );
    }

    qsort(dpa_best, _countof(dpa_best), sizeof(dpa_best[0]), qsort_cmp_trace_data_by_dpa_best);
    printf("\r\n%02X Diff      bit0     bit1     bit2     bit3     bit4     bit5     bit6     bit7\r\n", n_byte);
    for (unsigned int n = 0; n < 8; n++)
    {
        printf("%02x %f |%-8u %-8u %-8u %-8u %-8u %-8u %-8u %-8u|\r\n",
               dpa_best[n].n_byte,
               dpa_best[n].best_abs,
               pos_best[(0 * 256) + dpa_best[n].n_byte],
               pos_best[(1 * 256) + dpa_best[n].n_byte],
               pos_best[(2 * 256) + dpa_best[n].n_byte],
               pos_best[(3 * 256) + dpa_best[n].n_byte],
               pos_best[(4 * 256) + dpa_best[n].n_byte],
               pos_best[(5 * 256) + dpa_best[n].n_byte],
               pos_best[(6 * 256) + dpa_best[n].n_byte],
               pos_best[(7 * 256) + dpa_best[n].n_byte]);
    }

    free_pa_state(&pa, sad_array.empty());
}