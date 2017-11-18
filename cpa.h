#pragma once

int load_trace_file(std::string filepath, pa_state *pa);
void init_pa_state(pa_state *pa, int pa_type );
void cpa_process_byte(unsigned char byte_index, pa_state *pa);
void cpa_calculate_sum(pa_state *pa);
void free_pa_state(pa_state *pa, bool keep_points = false );
