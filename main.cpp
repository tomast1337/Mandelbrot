#include <X11/Xlib.h> // Ultiliza X11, fodase windows
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <fstream>
#include <iostream>
#include <stdlib.h>
#include <time.h>
#include <thread>
#include <string>

#define FrameRate 60
#define LOG(x) std::cout << x << std::endl;
#define processor_count std::thread::hardware_concurrency()

// Variaveis globais
int randR;
int randG;
int randB;
int color;
int imageWidth, imageHeight, maxN; // cria variaveis que serao lidas no arquivo
double minR, maxR, minI, maxI;

int calcMandelbrot(double cr, double ci, int max_iterations) // funcao que calcula Mandelbrot tem no wikipedia :P
{
    int i = 0;
    double zr = 0.0, zi = 0.0;
    while (i < max_iterations && zr * zr + zi * zi < 4.0)
    {
        double temp = zr * zr - zi * zi + cr;
        zi = 2.0 * zr * zi + ci;
        zr = temp;
        i++;
    }
    return i;
}

double mapToReal(int x, int imageWidth, double minR, double maxR) // funcao p/ calcular e atualizar valores
{
    double range = maxR - minR;
    return x * (range / imageWidth) + minR;
}

double maptoImaginary(int y, int imageHeight, double minI, double maxI)
{
    double range = maxI - minI;
    return y * (range / imageHeight) + minI;
}

void drawerThread(int *pixelBuffer, int bufferStart, int bufferEnd, int row, int y)
{
    for (int x = bufferStart; x < bufferEnd; x++)
    {
        double cr = mapToReal(x, imageWidth, minR, maxR);
        double ci = maptoImaginary(y, imageHeight, minI, maxI);
        int n = calcMandelbrot(cr, ci, maxN); // calcula numero de interações na funcao calcMandelbrot
        int r, g, b;
        if (color == 1) // se for colorido
        {
            r = (((n * randR)) % 256);
            g = ((n * randG) % 256);
            b = ((n * randB) % 256);
        }
        else if (color == 2) // se for preto e branco
        {
            r = (n % 256);
            g = (n % 256);
            b = (n % 256);
        }
        pixelBuffer[x + row * imageWidth] = (r << 16) | (g << 8) | b;
    }
}

void draw2file(int *pixelBuffer)
{
    std::ofstream fout("mandelbrot.ppm");                  // arquivo imagem final gerado (.ppm)
    fout << "P3" << std::endl;                             // magic number do arquivo (indica ppm)
    fout << imageWidth << " " << imageHeight << std::endl; // resolucao
    fout << "256" << std::endl;                            // valor maximo do rgb
    for (int i = 0; i < imageHeight * imageWidth; i++)
    {
        int r = (pixelBuffer[i] >> 16) & 0xFF;
        int g = (pixelBuffer[i] >> 8) & 0xFF;
        int b = (pixelBuffer[i]) & 0xFF;
        fout << r << " " << g << " " << b << std::endl;
    }
    fout.close();
    LOG("Finalizado! mandelbrot.ppm adicionado no diretorio atual");
}

void draw(GC gc, Display *display, Window win, int imageWidth, int imageHeight, double minR, double maxR, double minI, double maxI, int max_iterations)
{

    int *pixelBuffer = new int[imageWidth * imageHeight]; // buffer de pixels
    int threadCount = processor_count;                    // numero de threads
    for (int y = 0; y < imageHeight; y++)
    {
        int row = y;
        int bufferStart = 0;
        int bufferEnd = imageWidth;
        std::thread threads[threadCount];
        for (int i = 0; i < threadCount; i++)
        {
            threads[i] = std::thread(drawerThread, pixelBuffer, bufferStart, bufferEnd, row, y);
            bufferStart = bufferEnd;
            bufferEnd += imageWidth / threadCount;
        }
        for (int i = 0; i < threadCount; i++)
        {
            threads[i].join();
        }
        // Draw
        for (int x = 0; x < imageWidth; x++)
        {
            XSetForeground(display, gc, pixelBuffer[x + row * imageWidth]);
            XDrawPoint(display, win, gc, x, y);
        }
    }

    draw2file(pixelBuffer);
}

int initRender()
{
    Display *display;
    Window window;
    GC gc;
    XEvent event;

    display = XOpenDisplay(NULL);
    if (display == NULL)
    {
        LOG("Falha ao abrir o display");
        return EXIT_FAILURE;
    }

    int screen = DefaultScreen(display);
    window = XCreateSimpleWindow(display, RootWindow(display, screen), 0, 0, imageWidth, imageHeight, 0, 0, 0);
    XSelectInput(display, window, ExposureMask | KeyPressMask);
    XMapWindow(display, window);
    XStoreName(display, window, "Mandelbrot");

    gc = XCreateGC(display, window, 0, 0);
    XSetForeground(display, gc, 0xffffff);

    bool running = true;

    while (running)
    {
        XNextEvent(display, &event);
        switch (event.type)
        {
        case Expose:
            draw(gc, display, window, imageWidth, imageHeight, minR, maxR, minI, maxI, maxN);
            break;
        case KeyPress:
            running = false;
            break;
        }
    }

    return EXIT_SUCCESS;
}

int main()
{
    srand(time(NULL));    // seed para numero aleatorio (usa time.h)
    randR = rand() % 100; // cria variaveis com numeros aleatorios (para gerar mandelbrot colorido)
    randG = rand() % 100;
    randB = rand() % 100;

    while (color != 1 && color != 2)
    {
        LOG("----------------");
        LOG("Gerar Mandelbrot com:");
        LOG("1 - COR (aleatorio)\n2 - PRETO E BRANCO");
        LOG("----------------\n");
        std::cin >> color;
        if (color != 1 && color != 2)
        {
            LOG("----------------");
            LOG("Valor invalido!");
            LOG("----------------\n");
        }
    }

    std::ifstream fin("base.txt"); // le arquivo base para configuracao  para info de configuracao, va no help_base.txt
    if (!fin)
    {
        LOG("nao deu p abrir o arquivo");
        std::cin.ignore();
        return EXIT_FAILURE;
    }
    fin >> imageWidth >> imageHeight >> maxN;
    fin >> minR >> maxR >> minI >> maxI;

    LOG("----------------");
    LOG("imageWidth: " << imageWidth << "\nimageHeight: " << imageHeight);
    LOG("maxN: " << maxN
                 << "\nminR: " << minR
                 << "\nmaxR: " << maxR
                 << "\nminI: " << minI
                 << "\nmaxI: " << maxI);
    LOG("----------------\n");

    return initRender();
}
