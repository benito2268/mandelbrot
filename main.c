#include <ncurses.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/resource.h>

#define MAX_ITER 1000

static const char *shades = " .:-=+*!/?&#%@";

struct Pos {
    double center_x;
    double center_y;
    double scale;
};

void mb_init_colors() {
    start_color();
    use_default_colors();

    init_pair(1, COLOR_BLUE,   -1);
    init_pair(2, COLOR_WHITE,   -1);
    init_pair(3, COLOR_GREEN,  -1);
    init_pair(4, COLOR_YELLOW, -1);
    init_pair(5, COLOR_RED,    -1);
    init_pair(6, COLOR_MAGENTA,-1);
    init_pair(7, COLOR_CYAN,  -1);
}

double mb_now_sec(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec * 1e-6;
}

double mb_get_cpu(void) {
    static double last_wall = 0;
    static double last_cpu  = 0;

    struct rusage ru;
    getrusage(RUSAGE_SELF, &ru);

    double cpu =
        ru.ru_utime.tv_sec  + ru.ru_utime.tv_usec  * 1e-6 +
        ru.ru_stime.tv_sec  + ru.ru_stime.tv_usec  * 1e-6;

    double wall = mb_now_sec();

    double cpu_pct = 0.0;
    if (last_wall > 0) {
        cpu_pct = 100.0 * (cpu - last_cpu) / (wall - last_wall);
    }

    last_cpu  = cpu;
    last_wall = wall;
    return cpu_pct;
}

int mandelbrot(double cx, double cy) {
    double x = 0, y = 0;
    int iter = 0;

    while (x*x + y*y <= 4.0 && iter < MAX_ITER) {
        double x_t = x*x - y*y + cx;
        y = 2*x*y + cy;
        x = x_t;

        iter++;
    }

    return iter;
}

void mb_render(int rows, int cols, struct Pos *pos) {
    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {

            double cx = pos->center_x +
                (col - cols / 2.0) * pos->scale / cols;

            double cy = pos->center_y +
                (row - rows / 2.0) * pos->scale / cols;

            int iter = mandelbrot(cx, cy);

            int shade = (iter * 13) / MAX_ITER;
            int color = (iter * 6) / MAX_ITER + 1;

            attron(COLOR_PAIR(color));
            mvaddch(row, col, shades[shade]);
            attroff(COLOR_PAIR(color));
        }
    }
}

void mb_zoom_at(struct Pos *pos, double factor,
             int rows, int cols,
             int sx, int sy)
{
    double old_scale = pos->scale;
    pos->scale *= factor;

    double tx = pos->center_x +
        (sx - cols / 2.0) * old_scale / cols;

    double ty = pos->center_y +
        (sy - rows / 2.0) * old_scale / cols;

    pos->center_x = tx + (pos->center_x - tx) * (pos->scale / old_scale);
    pos->center_y = ty + (pos->center_y - ty) * (pos->scale / old_scale);
}

void mb_zoom_center(struct Pos *pos, double fac, int rows, int cols) {
    mb_zoom_at(pos, fac, rows, cols, cols / 2, rows / 2);
}


int main (void) {
    int row, col;
    int ch;
    struct Pos pos = {
        .center_x = -0.5,
        .center_y = 0.0,
        .scale = 3.0
    };

    // init the ncurses env
    initscr();

    // check for colors
    if (!has_colors()) {
        fprintf(stderr, "ERROR - terminal does not support colors :(\n");
        endwin();
        exit(1);
    }

    mb_init_colors();
    cbreak();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);
    getmaxyx(stdscr, row, col);
    scrollok(stdscr, TRUE);
    idlok(stdscr, TRUE);
    setscrreg(1, row - 1);

    // inital render
    mb_render(row, col, &pos);
 
    double cpu = 0.0;
    double next_cpu_update = 0.0;
    while ((ch = getch()) != 'q') {
        int mod = 0;
        double step = pos.scale / col;  // one pixel in complex space
        double pan  = step * 5;          // move 5 pixels per keypress

        switch (ch) {
            case KEY_UP:
                pos.center_y -= pan;
                mod = 1;
                break;
            case KEY_DOWN:
                pos.center_y += pan;
                mod = 1;
                break;
            case KEY_LEFT:
                pos.center_x -= pan;
                mod = 1;
                break;
            case KEY_RIGHT:
                pos.center_x += pan;
                mod = 1;
                break;
            case 'z':
                mb_zoom_center(&pos, 0.1, row, col);
                mod = 1;
                break;
            case 'x':
                mb_zoom_center(&pos, -1.0/0.1, row, col);
                mod = 1;
                mod = 1;
                break;
            default: break;
        } 
  
        if (mod) {
            clear();
            mb_render(row, col, &pos);
        } 

        double t = mb_now_sec();
        if (t < next_cpu_update) {
            cpu = mb_get_cpu();
            next_cpu_update = t + 2.0;
        }

        mvprintw(0, 0, "CPU: %5.1f%% x=%.6f cy=%.6f scale=%.6f", 
            cpu, pos.center_x, pos.center_y, pos.scale);
        refresh();
    }
    
    endwin();
    return 0;
}
