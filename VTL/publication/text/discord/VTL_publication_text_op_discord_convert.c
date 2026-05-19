#include <VTL/publication/text/discord/VTL_publication_text_op_discord_convert.h>
#include <VTL/publication/text/VTL_publication_text_data.h>
#include <stdlib.h>
#include <string.h>

/* =========================================================================
 * Discord MD -> VTL_publication_MarkedText
 * ========================================================================= */

/*
 * BoldItalicStart BOLD | ITALIC
 * BoldStart       BOLD
 * ItalicStart     ITALIC
 * StrikeStart     STRIKETHROUGH
 * *End            снимаем соответствующий флаг
 */
static VTL_publication_text_modification_Flags vtl_discord_flags_for_marker(
        VTL_discord_MarkerKind kind,
        VTL_publication_text_modification_Flags current) {
    switch (kind) {
        case VTL_discord_marker_kBoldStart:
            return current | VTL_TEXT_MODIFICATION_BOLD;
        case VTL_discord_marker_kBoldEnd:
            return current & ~VTL_TEXT_MODIFICATION_BOLD;
        case VTL_discord_marker_kItalicStart:
            return current | VTL_TEXT_MODIFICATION_ITALIC;
        case VTL_discord_marker_kItalicEnd:
            return current & ~VTL_TEXT_MODIFICATION_ITALIC;
        case VTL_discord_marker_kStrikeStart:
            return current | VTL_TEXT_MODIFICATION_STRIKETHROUGH;
        case VTL_discord_marker_kStrikeEnd:
            return current & ~VTL_TEXT_MODIFICATION_STRIKETHROUGH;
        case VTL_discord_marker_kBoldItalicStart:
            return current | VTL_TEXT_MODIFICATION_BOLD | VTL_TEXT_MODIFICATION_ITALIC;
        case VTL_discord_marker_kBoldItalicEnd:
            return current & ~(VTL_TEXT_MODIFICATION_BOLD | VTL_TEXT_MODIFICATION_ITALIC);
        default:
            return current;
    }
}

VTL_AppResult VTL_discord_ConvertToMarkedText(const VTL_publication_Text *src, const VTL_discord_MarkerList *markers, VTL_publication_MarkedText **out) {
    if (!src || !src->text || !markers || !out) {
        return VTL_res_kInvalidParamErr;
    }

    /* Кол-во макс. частей. Между каждой парой маркеров + хвост */
    size_t max_parts = markers->length + 1;

    VTL_publication_MarkedText *block = (VTL_publication_MarkedText *) malloc(sizeof(VTL_publication_MarkedText));
    if (!block) {
        return VTL_res_kMemAllocErr;
    }

    block->parts = (VTL_publication_marked_text_Part *) malloc(max_parts * sizeof(VTL_publication_marked_text_Part));
    if (!block->parts) {
        free(block);
        return VTL_res_kMemAllocErr;
    }
    block->length = 0;

    VTL_publication_text_modification_Flags cur_flags = 0;
    size_t text_pos = 0;

    for (size_t i = 0; i < markers->length; ++i) {
        const VTL_discord_Marker *m = &markers->items[i];

        /* Текст между предыдущим маркером и текущим */
        if (m->pos > text_pos) {
            VTL_publication_marked_text_Part *p = &block->parts[block->length++];
            p->text = src->text + text_pos;
            p->length = m->pos - text_pos;
            p->type = cur_flags;
        }

        /* Обновляем флаги и пропускаем сам токен */
        cur_flags = vtl_discord_flags_for_marker(m->kind, cur_flags);
        text_pos = m->pos + m->length;
    }

    /* Хвост после последнего маркера */
    if (text_pos < src->length) {
        VTL_publication_marked_text_Part *p = &block->parts[block->length++];
        p->text = src->text + text_pos;
        p->length = src->length - text_pos;
        p->type = cur_flags;
    }

    /* Если текст вообще без разметки */
    if (block->length == 0 && src->length > 0) {
        block->parts[block->length].text = src->text;
        block->parts[block->length].length = src->length;
        block->parts[block->length].type = 0;
        block->length = 1;
    }

    *out = block;
    return VTL_res_kOk;
}

/* =========================================================================
 * VTL_publication_MarkedText -> Discord MD
 * ========================================================================= */

/* Возвращает открывающую строку и её длину */
static const char *vtl_discord_open_tag(VTL_publication_text_modification_Flags f, size_t *out_len) {
    int bold = (f & VTL_TEXT_MODIFICATION_BOLD) != 0;
    int italic = (f & VTL_TEXT_MODIFICATION_ITALIC) != 0;
    int strike = (f & VTL_TEXT_MODIFICATION_STRIKETHROUGH) != 0;

    /* Порядок вложенности: strike снаружи, bold+italic внутри */
    if (strike && bold && italic) {
        *out_len = 5;
        return "~~***";
    }
    if (strike && bold) {
        *out_len = 4;
        return "~~**";
    }
    if (strike && italic) {
        *out_len = 3;
        return "~~*";
    }
    if (strike) {
        *out_len = 2;
        return "~~";
    }
    if (bold && italic) {
        *out_len = 3;
        return "***";
    }
    if (bold) {
        *out_len = 2;
        return "**";
    }
    if (italic) {
        *out_len = 1;
        return "*";
    }
    *out_len = 0;
    return "";
}

/* Закрывающий тег */
static const char *vtl_discord_close_tag(VTL_publication_text_modification_Flags f, size_t *out_len) {
    int bold = (f & VTL_TEXT_MODIFICATION_BOLD) != 0;
    int italic = (f & VTL_TEXT_MODIFICATION_ITALIC) != 0;
    int strike = (f & VTL_TEXT_MODIFICATION_STRIKETHROUGH) != 0;

    if (strike && bold && italic) {
        *out_len = 5;
        return "***~~";
    }
    if (strike && bold) {
        *out_len = 4;
        return "**~~";
    }
    if (strike && italic) {
        *out_len = 3;
        return "*~~";
    }
    if (strike) {
        *out_len = 2;
        return "~~";
    }
    if (bold && italic) {
        *out_len = 3;
        return "***";
    }
    if (bold) {
        *out_len = 2;
        return "**";
    }
    if (italic) {
        *out_len = 1;
        return "*";
    }
    *out_len = 0;
    return "";
}

VTL_AppResult VTL_discord_ConvertFromMarkedText(const VTL_publication_MarkedText *src, VTL_publication_Text **out) {
    if (!src || !out) {
        return VTL_res_kInvalidParamErr;
    }

    /* Подсчитываем максимальный размер выходной строки */
    size_t total = 0;
    for (size_t i = 0; i < src->length; ++i) {
        const VTL_publication_marked_text_Part *p = &src->parts[i];
        size_t open_len = 0, close_len = 0;
        vtl_discord_open_tag(p->type, &open_len);
        vtl_discord_close_tag(p->type, &close_len);
        total += open_len + p->length + close_len;
    }

    VTL_publication_Text *result = (VTL_publication_Text *) malloc(sizeof(VTL_publication_Text));
    if (!result) {
        return VTL_res_kMemAllocErr;
    }

    result->text = (VTL_publication_text_Symbol *) malloc(total + 1);
    if (!result->text) {
        free(result);
        return VTL_res_kMemAllocErr;
    }

    char *pos = result->text;
    for (size_t i = 0; i < src->length; ++i) {
        const VTL_publication_marked_text_Part *p = &src->parts[i];

        size_t open_len = 0, close_len = 0;
        const char *open_tag = vtl_discord_open_tag(p->type, &open_len);
        const char *close_tag = vtl_discord_close_tag(p->type, &close_len);

        memcpy(pos, open_tag, open_len);
        pos += open_len;
        memcpy(pos, p->text, p->length);
        pos += p->length;
        memcpy(pos, close_tag, close_len);
        pos += close_len;
    }
    *pos = '\0';

    result->length = (size_t)(pos - result->text);
    *out = result;
    return VTL_res_kOk;
}
