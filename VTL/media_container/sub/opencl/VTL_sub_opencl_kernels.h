#ifndef VTL_SUB_OPENCL_KERNELS_H
#define VTL_SUB_OPENCL_KERNELS_H

/*
 * Исходники OpenCL-ядер вынесены в отдельный заголовок.
 *
 * В оригинале каждый .c файл объявлял своё ядро через #define без значения:
 *
 *   #define VTL_SUB_OPENCL_KERNEL_SOURCE_STRIP_TAGS
 *   "__kernel void strip_tags(...) { ... }\n";
 *
 * Это не работает: препроцессор определяет макрос в пустоту, а строки ниже
 * становятся висящими выражениями, никуда не привязанными. Когда потом пишется
 *   clCreateProgramWithSource(..., &VTL_SUB_OPENCL_KERNEL_SOURCE_STRIP_TAGS, ...)
 * передаётся адрес пустого макроса — UB, ядро не компилируется.
 *
 * Теперь каждый исходник — static const char*, которую корректно можно взять
 * по адресу и передать в clCreateProgramWithSource.
 */

static const char* VTL_SUB_OPENCL_KERNEL_SOURCE_STRIP_TAGS =
        "__kernel void strip_tags(__global const char* in_data, __global int* offsets, __global int* lengths, __global char* out_data, __global int* out_offsets) {\n"
        "    int idx = get_global_id(0);\n" // Индекс текущей строки
        "    int in_offset = offsets[idx];\n" // Смещение начала строки во входном буфере
        "    int in_len = lengths[idx];\n"   // Длина строки
        "    int out_offset = out_offsets[idx];\n" // Смещение для записи результата
        "    int in_tag = 0;\n" // Флаг: находимся ли внутри тега
        "    int j = 0;\n" // Индекс для записи в выходной буфер
        "    for (int k = 0; k < in_len; ++k) {\n"
        "        char c = in_data[in_offset + k];\n" // Читаем символ
        "        if (c == '{') in_tag = 1;\n" // Начало тега
        "        else if (c == '}' && in_tag) in_tag = 0;\n" // Конец тега
        "        else if (!in_tag) out_data[out_offset + j++] = c;\n" // Если не в теге — копируем символ
        "    }\n"
        "    out_data[out_offset + j] = '\\0';\n" // Завершаем строку нулём
        "}\n";

static const char* VTL_SUB_OPENCL_KERNEL_SOURCE_CLEAN_TAGS =
        "__kernel void clean_tags(__global const char* in_data, __global int* offsets, __global int* lengths, __global char* out_data, __global int* out_offsets) {\n"
        "    int idx = get_global_id(0);\n"
        "    int in_offset = offsets[idx];\n"
        "    int in_len = lengths[idx];\n"
        "    int out_offset = out_offsets[idx];\n"
        "    int in_tag = 0;\n"
        "    int j = 0;\n"
        "    for (int k = 0; k < in_len; ++k) {\n"
        "        char c = in_data[in_offset + k];\n"
        "        // Удаляем HTML/SGML/ASS-теги\n"
        "        if (c == '{' || c == '<') in_tag = 1;\n"
        "        else if ((c == '}' || c == '>') && in_tag) in_tag = 0;\n"
        "        else if (!in_tag) {\n"
        "            // Удаляем спецсимволы (упрощённо: не ASCII-печатаемые)\n"
        "            if ((unsigned char)c < 32 || (unsigned char)c > 126) continue;\n"
        "            // Можно добавить фильтр по диапазонам Unicode для эмодзи\n"
        "            out_data[out_offset + j++] = c;\n"
        "        }\n"
        "    }\n"
        "    out_data[out_offset + j] = '\\0';\n"
        "}\n";

static const char* VTL_SUB_OPENCL_KERNEL_SOURCE_DETECT_ENCODING =
        "__kernel void detect_encoding(__global const char* in_data, __global int* offsets, __global int* lengths, __global int* out_encodings) {\n"
        "    int idx = get_global_id(0);\n"
        "    int in_offset = offsets[idx];\n"
        "    int in_len = lengths[idx];\n"
        "    int encoding = 4; // неизвестно\n"
        "    int ascii = 1;\n"
        "    int latin1 = 1;\n"
        "    int win1251 = 1;\n"
        "    int utf8 = 1;\n"
        "    int i = 0;\n"
        "    while (i < in_len) {\n"
        "        unsigned char c = in_data[in_offset + i];\n"
        "        if (c < 0x80) { i++; continue; }\n"
        "        ascii = 0;\n"
        "        // UTF-8 check\n"
        "        if ((c & 0xE0) == 0xC0 && i+1 < in_len && (in_data[in_offset+i+1] & 0xC0) == 0x80) { i+=2; continue; }\n"
        "        if ((c & 0xF0) == 0xE0 && i+2 < in_len && (in_data[in_offset+i+1] & 0xC0) == 0x80 && (in_data[in_offset+i+2] & 0xC0) == 0x80) { i+=3; continue; }\n"
        "        if ((c & 0xF8) == 0xF0 && i+3 < in_len && (in_data[in_offset+i+1] & 0xC0) == 0x80 && (in_data[in_offset+i+2] & 0xC0) == 0x80 && (in_data[in_offset+i+3] & 0xC0) == 0x80) { i+=4; continue; }\n"
        "        utf8 = 0;\n"
        "        // Latin1 check: 0xA0-0xFF\n"
        "        if (!(c >= 0xA0 && c <= 0xFF)) latin1 = 0;\n"
        "        // Win1251 check: 0xC0-0xFF, 0x80-0xBF\n"
        "        if (!((c >= 0xC0 && c <= 0xFF) || (c >= 0x80 && c <= 0xBF))) win1251 = 0;\n"
        "        i++;\n"
        "    }\n"
        "    if (utf8) encoding = 0;\n"
        "    else if (win1251) encoding = 1;\n"
        "    else if (latin1) encoding = 2;\n"
        "    else if (ascii) encoding = 3;\n"
        "    out_encodings[idx] = encoding;\n"
        "}\n";

static const char* VTL_SUB_OPENCL_KERNEL_SOURCE_SPLIT_LONG_LINES =
        "__kernel void split_long_lines(__global const char* in_data, __global int* offsets, __global int* lengths, __global int* max_len, __global int* out_offsets, __global int* out_lengths, __global int* out_counts) {\n"
        "    int idx = get_global_id(0);\n"
        "    int in_offset = offsets[idx];\n"
        "    int in_len = lengths[idx];\n"
        "    int mlen = max_len[idx];\n"
        "    int out_off = out_offsets[idx];\n"
        "    int out_count = 0;\n"
        "    int pos = 0;\n"
        "    while (pos < in_len) {\n"
        "        int end = pos + mlen;\n"
        "        if (end > in_len) end = in_len;\n"
        "        int split = end;\n"
        "        // ищем последний пробел до split\n"
        "        for (int i = end - 1; i > pos; --i) {\n"
        "            if (in_data[in_offset + i] == ' ') { split = i; break; }\n"
        "        }\n"
        "        if (split == pos) split = end; // если пробела нет, режем по max_len\n"
        "        out_lengths[out_off + out_count] = split - pos;\n"
        "        out_count++;\n"
        "        pos = split;\n"
        "        while (pos < in_len && in_data[in_offset + pos] == ' ') pos++; // пропускаем пробелы\n"
        "    }\n"
        "    out_counts[idx] = out_count;\n"
        "}\n";

static const char* VTL_SUB_OPENCL_KERNEL_SOURCE_APPLY_ASS_STYLE =
        "__kernel void apply_ass_style(__global const char* in_data, __global int* offsets, __global int* lengths, __global char* out_data, __global int* out_offsets, __global const char* style_tag, int tag_len, int mode) {\n"
        "    int idx = get_global_id(0);\n" // Каждый рабочий элемент idx отвечает за одну входную строку.
        "    int in_offset = offsets[idx];\n" // Смещение входной строки в общем буфере.
        "    int in_len = lengths[idx];\n" // Длина входной строки.
        "    int out_offset = out_offsets[idx];\n" // Смещение выходной строки в общем буфере.
        "    if (mode == 1) { // добавить стиль\n"
        "        // Копируем style_tag в начало\n"
        "        for (int i = 0; i < tag_len; ++i) out_data[out_offset + i] = style_tag[i];\n"
        "        // Копируем строку\n"
        "        for (int i = 0; i < in_len; ++i) out_data[out_offset + tag_len + i] = in_data[in_offset + i];\n"
        "        // Копируем style_tag в конец\n"
        "        for (int i = 0; i < tag_len; ++i) out_data[out_offset + tag_len + in_len + i] = style_tag[i];\n"
        "        out_data[out_offset + tag_len + in_len + tag_len] = '\\0';\n"
        "    } else { // удалить стиль\n"
        "        int j = 0, k = 0;\n"
        "        while (k < in_len) {\n"
        "            int match = 1;\n"
        "            for (int t = 0; t < tag_len && k + t < in_len; ++t) {\n"
        "                if (in_data[in_offset + k + t] != style_tag[t]) { match = 0; break; }\n"
        "            }\n"
        "            if (match && tag_len > 0 && k + tag_len <= in_len) { k += tag_len; }\n"
        "            else out_data[out_offset + j++] = in_data[in_offset + k++];\n"
        "        }\n"
        "        out_data[out_offset + j] = '\\0';\n"
        "    }\n"
        "}\n";

static const char* VTL_SUB_OPENCL_KERNEL_SOURCE_STRING_STATS =
        "typedef struct {\n"
        "    uint length;\n"
        "    uint word_count;\n"
        "    uint unique_chars;\n"
        "    uint char_freq[128];\n"
        "} VTL_StringStats_CL;\n"
        "__kernel void string_stats(__global const char* in_data, __global int* offsets, __global int* lengths, __global VTL_StringStats_CL* out_stats) {\n"
        "    int idx = get_global_id(0);\n"
        "    int in_offset = offsets[idx];\n"
        "    int in_len = lengths[idx];\n"
        "    VTL_StringStats_CL stats = {0, 0, 0, {0}};\n"
        "    stats.length = in_len;\n"
        "    char prev_space = 1;\n"
        "    uchar seen[128] = {0};\n"
        "    for (int k = 0; k < in_len; ++k) {\n"
        "        char c = in_data[in_offset + k];\n"
        "        if ((unsigned char)c < 128) {\n"
        "            stats.char_freq[(int)(unsigned char)c]++;\n"
        "            if (!seen[(int)(unsigned char)c]) { seen[(int)(unsigned char)c] = 1; stats.unique_chars++; }\n"
        "        }\n"
        "        if ((c == ' ' || c == '\\t' || c == '\\n' || c == '\\r') && !prev_space) prev_space = 1;\n"
        "        else if (!(c == ' ' || c == '\\t' || c == '\\n' || c == '\\r') && prev_space) { stats.word_count++; prev_space = 0; }\n"
        "    }\n"
        "    out_stats[idx] = stats;\n"
        "}\n";

static const char* VTL_SUB_OPENCL_KERNEL_SOURCE_HASH_STRINGS =
        "__kernel void hash_strings(__global const char* in_data, __global int* offsets, __global int* lengths, __global uint* out_hashes) {\n"
        "    int idx = get_global_id(0);\n"
        "    int in_offset = offsets[idx];\n"
        "    int in_len = lengths[idx];\n"
        "    uint crc = 0xFFFFFFFFU;\n"
        "    for (int k = 0; k < in_len; ++k) {\n"
        "        unsigned char c = in_data[in_offset + k];\n"
        "        crc ^= c;\n"
        "        for (int j = 0; j < 8; ++j) {\n"
        "            uint mask = -(crc & 1U);\n"
        "            crc = (crc >> 1) ^ (0xEDB88320U & mask);\n"
        "        }\n"
        "    }\n"
        "    out_hashes[idx] = ~crc;\n"
        "}\n";

static const char* VTL_SUB_OPENCL_KERNEL_SOURCE_SQUEEZE_REPEATS =
        "__kernel void squeeze_repeats(__global const char* in_data, __global int* offsets, __global int* lengths, __global char* out_data, __global int* out_offsets) {\n"
        "    int idx = get_global_id(0);\n"
        "    int in_offset = offsets[idx];\n"
        "    int in_len = lengths[idx];\n"
        "    int out_offset = out_offsets[idx];\n"
        "    int j = 0;\n"
        "    char prev = 0;\n"
        "    for (int k = 0; k < in_len; ++k) {\n"
        "        char c = in_data[in_offset + k];\n"
        "        if (k == 0 || c != prev) {\n"
        "            out_data[out_offset + j++] = c;\n"
        "            prev = c;\n"
        "        }\n"
        "    }\n"
        "    out_data[out_offset + j] = '\\0';\n"
        "}\n";

static const char* VTL_SUB_OPENCL_KERNEL_SOURCE_FORMAT_NUMBERS =
        "__kernel void format_numbers(__global const char* in_data, __global int* offsets, __global int* lengths, __global char* out_data, __global int* out_offsets, char sep, int group_len) {\n"
        "    int idx = get_global_id(0);\n"
        "    int in_offset = offsets[idx];\n"
        "    int in_len = lengths[idx];\n"
        "    int out_offset = out_offsets[idx];\n"
        "    int j = 0;\n"
        "    int i = 0;\n"
        "    while (i < in_len) {\n"
        "        // Копируем нечисловые символы\n"
        "        if (in_data[in_offset + i] < '0' || in_data[in_offset + i] > '9') {\n"
        "            out_data[out_offset + j++] = in_data[in_offset + i++];\n"
        "            continue;\n"
        "        }\n"
        "        // Найдена последовательность цифр\n"
        "        int num_start = i;\n"
        "        while (i < in_len && in_data[in_offset + i] >= '0' && in_data[in_offset + i] <= '9') i++;\n"
        "        int num_len = i - num_start;\n"
        "        // Копируем цифры с разделителями справа налево\n"
        "        for (int k = 0; k < num_len; ++k) {\n"
        "            if (k > 0 && k % group_len == 0) out_data[out_offset + j++] = sep;\n"
        "            out_data[out_offset + j++] = in_data[in_offset + num_start + num_len - 1 - k];\n"
        "        }\n"
        "        // Разворачиваем обратно\n"
        "        for (int k = 0; k < (j - (out_offset + j - num_len - (num_len-1)/group_len)); ++k) {\n"
        "            char tmp = out_data[out_offset + j - k - 1];\n"
        "            out_data[out_offset + j - k - 1] = out_data[out_offset + j - num_len - (num_len-1)/group_len + k];\n"
        "            out_data[out_offset + j - num_len - (num_len-1)/group_len + k] = tmp;\n"
        "        }\n"
        "    }\n"
        "    out_data[out_offset + j] = '\\0';\n"
        "}\n";

/* sprintf недоступен в OpenCL C — записываем цифры вручную */
static const char* VTL_SUB_OPENCL_KERNEL_SOURCE_FORMAT_TIME =
        "__kernel void format_time(__global const double* in_times, __global char* out_data, __global int* out_offsets, int format) {\n"
        "    int idx = get_global_id(0);\n"
        "    double t = in_times[idx];\n"
        "    int hours   = (int)(t / 3600.0);\n"
        "    int minutes = (int)((t - hours * 3600.0) / 60.0);\n"
        "    int seconds = (int)(t) % 60;\n"
        "    int ms = (int)((t - (int)t) * 1000.0 + 0.5);\n"
        "    int cs = (int)((t - (int)t) * 100.0  + 0.5);\n"
        "    __global char* o = out_data + out_offsets[idx];\n"
        "    if (format == 1) {\n"
        "        o[0]=(char)('0'+hours); o[1]=':';\n"
        "        o[2]=(char)('0'+minutes/10); o[3]=(char)('0'+minutes%10); o[4]=':';\n"
        "        o[5]=(char)('0'+seconds/10); o[6]=(char)('0'+seconds%10); o[7]='.';\n"
        "        o[8]=(char)('0'+cs/10); o[9]=(char)('0'+cs%10); o[10]='\\0';\n"
        "    } else {\n"
        "        o[0]=(char)('0'+hours/10);   o[1]=(char)('0'+hours%10);   o[2]=':';\n"
        "        o[3]=(char)('0'+minutes/10); o[4]=(char)('0'+minutes%10); o[5]=':';\n"
        "        o[6]=(char)('0'+seconds/10); o[7]=(char)('0'+seconds%10);\n"
        "        o[8]=(format==0)?',':'.';\n"
        "        o[9]=(char)('0'+ms/100); o[10]=(char)('0'+(ms/10)%10); o[11]=(char)('0'+ms%10); o[12]='\\0';\n"
        "    }\n"
        "}\n";
#endif /* VTL_SUB_OPENCL_KERNELS_H */