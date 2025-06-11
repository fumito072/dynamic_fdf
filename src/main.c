#define GL_SILENCE_DEPRECATION
#define OPENAL_DEPRECATED
#include <GLFW/glfw3.h>
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <sys/time.h>

#define NUM_LINES 1

int load_wav(const char* filename, ALuint* buffer) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        perror("fopen");
        return 0;
    }

    char chunk[4];
    fread(chunk, 1, 4, file); // "RIFF"
    fseek(file, 8, SEEK_SET);
    fread(chunk, 1, 4, file); // "WAVE"

    // スキップしながらfmtチャンクとdataチャンク探す（簡易）
    short formatType, channels;
    int sampleRate, dataSize;
    short bitsPerSample;
    char subchunk[4];
    int subchunkSize;

    fread(subchunk, 1, 4, file); // "fmt "
    fread(&subchunkSize, 4, 1, file);
    fread(&formatType, 2, 1, file);
    fread(&channels, 2, 1, file);
    fread(&sampleRate, 4, 1, file);
    fseek(file, 6, SEEK_CUR); // skip byte rate, block align
    fread(&bitsPerSample, 2, 1, file);
    fseek(file, subchunkSize - 16, SEEK_CUR); // skip rest of fmt

    fread(subchunk, 1, 4, file); // "data"
    fread(&dataSize, 4, 1, file);

    char* data = malloc(dataSize);
    fread(data, 1, dataSize, file);
    fclose(file);

    ALenum format;
    if (channels == 1 && bitsPerSample == 8) format = AL_FORMAT_MONO8;
    else if (channels == 1 && bitsPerSample == 16) format = AL_FORMAT_MONO16;
    else if (channels == 2 && bitsPerSample == 8) format = AL_FORMAT_STEREO8;
    else if (channels == 2 && bitsPerSample == 16) format = AL_FORMAT_STEREO16;
    else {
        printf("Unsupported audio format\n");
        free(data);
        return 0;
    }

    alGenBuffers(1, buffer);
    alBufferData(*buffer, format, data, dataSize, sampleRate);
    free(data);
    return 1;
}

// 音を再生する関数
ALuint init_and_play_audio(const char* path) {
    ALCdevice* device = alcOpenDevice(NULL);
    ALCcontext* context = alcCreateContext(device, NULL);
    alcMakeContextCurrent(context);

    ALuint buffer;
    if (!load_wav(path, &buffer)) {
        printf("Failed to load WAV\n");
        exit(1);
    }

    ALuint source;
    alGenSources(1, &source);
    alSourcei(source, AL_BUFFER, buffer);
    alSourcei(source, AL_LOOPING, AL_TRUE); // 無限ループ
    alSourcePlay(source);

    return source;
}

void error_callback(int error, const char* description){
    fprintf(stderr, "Error: %s\n", description);
}

void isometric_project(float x, float y, float z, float *out_x, float *out_y){
    float angle = M_PI / 6;
    *out_x = (x - y) * cos(angle);
    *out_y = (x + y) * sin(angle) - z;
}

double get_time_sec(){
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)(tv.tv_sec) + (double)(tv.tv_usec) / 1000000.0;
}

float get_audio_time(ALuint source) {
    float seconds;
    alGetSourcef(source, AL_SEC_OFFSET, &seconds);
    return seconds;
}

int main(){
    GLFWwindow *window;

    glfwSetErrorCallback(error_callback);    
    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);

    window = glfwCreateWindow(800, 600, "Line Drawing - C OpenGL", NULL, NULL);
    if (!window){
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    ALuint source = init_and_play_audio("assets/noise.wav");

    float x_start[NUM_LINES], y_start[NUM_LINES];
    float freq[NUM_LINES], phase[NUM_LINES], color_r[NUM_LINES], color_g[NUM_LINES], color_b[NUM_LINES];

    for (int i = 0; i < NUM_LINES; i++) {
        x_start[i] = ((float)rand() / RAND_MAX) * 800.0f - 400.0f;
        y_start[i] = ((float)rand() / RAND_MAX) * 600.0f - 300.0f;
        freq[i] = 0.5f + ((float)rand() / RAND_MAX) * 3.0f;
        phase[i] = ((float)rand() / RAND_MAX) * 2.0f * M_PI;
        color_r[i] = ((float)rand() / RAND_MAX) * 0.8f + 0.2f;
        color_g[i] = ((float)rand() / RAND_MAX) * 0.8f + 0.2f;
        color_b[i] = ((float)rand() / RAND_MAX) * 0.8f + 0.2f;
    }

    while (!glfwWindowShouldClose(window)){
        glClearColor(0.0, 0.0, 0.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(-400, 400, -300, 300, -1, 1);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        float t = get_audio_time(source);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        float zoom = 1.0f + sin(t * 0.5f) * 0.3f;
        glOrtho(-400 * zoom, 400 * zoom, -300 * zoom, 300 * zoom, -1, 1);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        float camera_angle = t * 0.001f;
        glRotatef(camera_angle, 0.0f, 1.0f, 0.0f);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);


        for (int i = 0; i < NUM_LINES; i++) {
            float alpha = 0.2f + fabs(sin(t * freq[i] + phase[i])) * 0.5f;
            float thickness = 1.0f + fabs(sin(t * freq[i])) * 3.0f;
            glLineWidth(thickness);
            glColor4f(color_r[i], color_g[i], color_b[i], alpha);
            float length = 60.0f + sin(t * freq[i] + phase[i]) * 40.0f;
            float z = sin(t * freq[i] + phase[i] + 1.0f) * 30.0f;

            float sx1, sy1, sx2, sy2;
            isometric_project(x_start[i], y_start[i], 0, &sx1, &sy1);
            isometric_project(x_start[i], y_start[i] + length, z, &sx2, &sy2);

            glColor3f(color_r[i], color_g[i], color_b[i]);
            glBegin(GL_LINES);
                glVertex2f(sx1, sy1);
                glVertex2f(sx2, sy2);
            glEnd();
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}