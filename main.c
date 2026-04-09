#include <VTL/VTL.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(void)
{
    const char* audio_files[] = {
        "audio_ariel.mp3",
        "audio_styuardessa.mp3",
        "audio_xanadu.mp3"
    };
    srand((unsigned)time(NULL));
    int pick = rand() % 3;

    /* --- 1. Текстовый пайплайн --- */
    printf("=== Text Pipeline ===\n");
    VTL_AppResult res_text = VTL_PubicateMarkedText("text.md",
        VTL_CONTENT_PLATFORM_W | VTL_CONTENT_PLATFORM_TG,
        VTL_markup_type_kTelegramMD);
    printf("Text: %d (%s)\n\n", res_text,
        res_text == VTL_res_kOk ? "OK" : "ERROR");

    /* --- 2. Аудио + текст пайплайн (случайный трек) --- */
    printf("=== Audio Pipeline [%d]: %s ===\n", pick, audio_files[pick]);
    VTL_AppResult res_audio = VTL_PubicateAudioWithMarkedText(
        audio_files[pick], "text.md",
        VTL_markup_type_kTelegramMD,
        VTL_CONTENT_PLATFORM_W | VTL_CONTENT_PLATFORM_TG);
    printf("Audio: %d (%s)\n", res_audio,
        res_audio == VTL_res_kOk ? "OK" : "ERROR");

    return (res_text != VTL_res_kOk) ? res_text : res_audio;
}