// This is the ex_polygon example program from allegro 5,
// but with a trivial allegro4 usage to test the allegro_legacy setup.

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_primitives.h>
#include <allegro.h>
#include <a5alleg.h>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#endif

void abort_example(char const *format, ...)
{
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  exit(1);
}

void open_log(void)
{
}

void open_log_monospace(void)
{
}

void close_log(bool wait_for_user)
{
  (void)wait_for_user;
}

void log_printf(char const *format, ...)
{
  va_list args;
  va_start(args, format);
#ifdef ALLEGRO_ANDROID
  char x[1024];
  vsnprintf(x, sizeof x, format, args);
  ALLEGRO_TRACE_CHANNEL_LEVEL("log", 1)
  ("%s", x);
#else
  vfprintf(stderr, format, args);
#endif
  va_end(args);
}

/*
 *    Example program for the Allegro library, by Peter Wang.
 *
 *    This program tests the polygon routines in the primitives addon.
 */

#define MAX_VERTICES 64
#define MAX_POLYGONS 9
#define RADIUS 5

enum
{
  MODE_POLYLINE = 0,
  MODE_POLYGON,
  MODE_FILLED_POLYGON,
  MODE_FILLED_HOLES,
  MODE_MAX
};

enum AddHole
{
  NOT_ADDING_HOLE,
  NEW_HOLE,
  GROW_HOLE
};

typedef struct Vertex Vertex;
struct Vertex
{
  float x;
  float y;
};

struct Example
{
  ALLEGRO_DISPLAY *display;
  ALLEGRO_FONT *font;
  ALLEGRO_FONT *fontbmp;
  ALLEGRO_EVENT_QUEUE *queue;
  ALLEGRO_BITMAP *dbuf;
  ALLEGRO_COLOR bg;
  ALLEGRO_COLOR fg;
  Vertex vertices[MAX_VERTICES];
  int vertex_polygon[MAX_VERTICES];
  int vertex_count;
  int cur_vertex; /* -1 = none */
  int cur_polygon;
  int mode;
  ALLEGRO_LINE_CAP cap_style;
  ALLEGRO_LINE_JOIN join_style;
  float thickness;
  float miter_limit;
  bool software;
  float zoom;
  float scroll_x, scroll_y;
  enum AddHole add_hole;
  bool mdown;
};

static struct Example ex;

static void reset(void)
{
  ex.vertex_count = 0;
  ex.cur_vertex = -1;
  ex.cur_polygon = 0;
  ex.cap_style = ALLEGRO_LINE_CAP_NONE;
  ex.join_style = ALLEGRO_LINE_JOIN_NONE;
  ex.thickness = 1.0f;
  ex.miter_limit = 1.0f;
  ex.software = false;
  ex.zoom = 1;
  ex.scroll_x = 0;
  ex.scroll_y = 0;
  ex.add_hole = NOT_ADDING_HOLE;
}

static void transform(float *x, float *y)
{
  *x /= ex.zoom;
  *y /= ex.zoom;
  *x -= ex.scroll_x;
  *y -= ex.scroll_y;
}

static int hit_vertex(int mx, int my)
{
  int i;

  for (i = 0; i < ex.vertex_count; i++)
  {
    int dx = ex.vertices[i].x - mx;
    int dy = ex.vertices[i].y - my;
    int dd = dx * dx + dy * dy;
    if (dd <= RADIUS * RADIUS)
      return i;
  }

  return -1;
}

static void lclick(int mx, int my)
{
  ex.cur_vertex = hit_vertex(mx, my);

  if (ex.cur_vertex < 0 && ex.vertex_count < MAX_VERTICES)
  {
    int i = ex.vertex_count++;
    ex.vertices[i].x = mx;
    ex.vertices[i].y = my;
    ex.cur_vertex = i;

    if (ex.add_hole == NEW_HOLE && ex.cur_polygon < MAX_POLYGONS)
    {
      ex.cur_polygon++;
      ex.add_hole = GROW_HOLE;
    }

    ex.vertex_polygon[i] = ex.cur_polygon;
  }
}

static void rclick(int mx, int my)
{
  const int i = hit_vertex(mx, my);
  if (i >= 0 && ex.add_hole == NOT_ADDING_HOLE)
  {
    ex.vertex_count--;
    memmove(&ex.vertices[i], &ex.vertices[i + 1],
            sizeof(Vertex) * (ex.vertex_count - i));
    memmove(&ex.vertex_polygon[i], &ex.vertex_polygon[i + 1],
            sizeof(int) * (ex.vertex_count - i));
  }
  ex.cur_vertex = -1;
}

static void drag(int mx, int my)
{
  if (ex.cur_vertex >= 0)
  {
    ex.vertices[ex.cur_vertex].x = mx;
    ex.vertices[ex.cur_vertex].y = my;
  }
}

static void scroll(int mx, int my)
{
  ex.scroll_x += mx;
  ex.scroll_y += my;
}

static const char *join_style_to_string(ALLEGRO_LINE_JOIN x)
{
  switch (x)
  {
  case ALLEGRO_LINE_JOIN_NONE:
    return "NONE";
  case ALLEGRO_LINE_JOIN_BEVEL:
    return "BEVEL";
  case ALLEGRO_LINE_JOIN_ROUND:
    return "ROUND";
  case ALLEGRO_LINE_JOIN_MITER:
    return "MITER";
  default:
    return "unknown";
  }
}

static const char *cap_style_to_string(ALLEGRO_LINE_CAP x)
{
  switch (x)
  {
  case ALLEGRO_LINE_CAP_NONE:
    return "NONE";
  case ALLEGRO_LINE_CAP_SQUARE:
    return "SQUARE";
  case ALLEGRO_LINE_CAP_ROUND:
    return "ROUND";
  case ALLEGRO_LINE_CAP_TRIANGLE:
    return "TRIANGLE";
  case ALLEGRO_LINE_CAP_CLOSED:
    return "CLOSED";
  default:
    return "unknown";
  }
}

static ALLEGRO_FONT *choose_font(void)
{
  return (ex.software) ? ex.fontbmp : ex.font;
}

static void draw_vertices(void)
{
  ALLEGRO_FONT *f = choose_font();
  ALLEGRO_COLOR vertc = al_map_rgba_f(0.7, 0, 0, 0.7);
  ALLEGRO_COLOR textc = al_map_rgba_f(0, 0, 0, 0.7);
  int i;

  for (i = 0; i < ex.vertex_count; i++)
  {
    float x = ex.vertices[i].x;
    float y = ex.vertices[i].y;

    al_draw_filled_circle(x, y, RADIUS, vertc);
    al_draw_textf(f, textc, x + RADIUS, y + RADIUS, 0, "%d", i);
  }
}

static void compute_polygon_vertex_counts(
    int polygon_vertex_count[MAX_POLYGONS + 1])
{
  int i;

  /* This also implicitly terminates the array with a zero. */
  memset(polygon_vertex_count, 0, sizeof(int) * (MAX_POLYGONS + 1));
  for (i = 0; i < ex.vertex_count; i++)
  {
    const int poly = ex.vertex_polygon[i];
    polygon_vertex_count[poly]++;
  }
}

static void draw_all(void)
{
  ALLEGRO_FONT *f = choose_font();
  ALLEGRO_COLOR textc = al_map_rgb(0, 0, 0);
  float texth = al_get_font_line_height(f) * 1.5;
  float textx = 5;
  float texty = 5;
  ALLEGRO_TRANSFORM t;
  ALLEGRO_COLOR holec;

  al_clear_to_color(ex.bg);

  al_identity_transform(&t);
  al_translate_transform(&t, ex.scroll_x, ex.scroll_y);
  al_scale_transform(&t, ex.zoom, ex.zoom);
  al_use_transform(&t);

  if (ex.mode == MODE_POLYLINE)
  {
    if (ex.vertex_count >= 2)
    {
      al_draw_polyline(
          (float *)ex.vertices, sizeof(Vertex), ex.vertex_count,
          ex.join_style, ex.cap_style, ex.fg, ex.thickness, ex.miter_limit);
    }
  }
  else if (ex.mode == MODE_FILLED_POLYGON)
  {
    if (ex.vertex_count >= 2)
    {
      al_draw_filled_polygon(
          (float *)ex.vertices, ex.vertex_count, ex.fg);
    }
  }
  else if (ex.mode == MODE_POLYGON)
  {
    if (ex.vertex_count >= 2)
    {
      al_draw_polygon(
          (float *)ex.vertices, ex.vertex_count,
          ex.join_style, ex.fg, ex.thickness, ex.miter_limit);
    }
  }
  else if (ex.mode == MODE_FILLED_HOLES)
  {
    if (ex.vertex_count >= 2)
    {
      int polygon_vertex_count[MAX_POLYGONS + 1];
      compute_polygon_vertex_counts(polygon_vertex_count);
      al_draw_filled_polygon_with_holes(
          (float *)ex.vertices, polygon_vertex_count, ex.fg);
    }
  }

  draw_vertices();

  al_identity_transform(&t);
  al_use_transform(&t);

  if (ex.mode == MODE_POLYLINE)
  {
    al_draw_textf(f, textc, textx, texty, 0,
                  "al_draw_polyline (SPACE)");
    texty += texth;
  }
  else if (ex.mode == MODE_FILLED_POLYGON)
  {
    al_draw_textf(f, textc, textx, texty, 0,
                  "al_draw_filled_polygon (SPACE)");
    texty += texth;
  }
  else if (ex.mode == MODE_POLYGON)
  {
    al_draw_textf(f, textc, textx, texty, 0,
                  "al_draw_polygon (SPACE)");
    texty += texth;
  }
  else if (ex.mode == MODE_FILLED_HOLES)
  {
    al_draw_textf(f, textc, textx, texty, 0,
                  "al_draw_filled_polygon_with_holes (SPACE)");
    texty += texth;
  }

  al_draw_textf(f, textc, textx, texty, 0,
                "Line join style: %s (J)", join_style_to_string(ex.join_style));
  texty += texth;

  al_draw_textf(f, textc, textx, texty, 0,
                "Line cap style:  %s (C)", cap_style_to_string(ex.cap_style));
  texty += texth;

  al_draw_textf(f, textc, textx, texty, 0,
                "Line thickness:  %.2f (+/-)", ex.thickness);
  texty += texth;

  al_draw_textf(f, textc, textx, texty, 0,
                "Miter limit:     %.2f ([/])", ex.miter_limit);
  texty += texth;

  al_draw_textf(f, textc, textx, texty, 0,
                "Zoom:            %.2f (wheel)", ex.zoom);
  texty += texth;

  al_draw_textf(f, textc, textx, texty, 0,
                "%s (S)", (ex.software ? "Software rendering" : "Hardware rendering"));
  texty += texth;

  al_draw_textf(f, textc, textx, texty, 0,
                "Reset (R)");
  texty += texth;

  if (ex.add_hole == NOT_ADDING_HOLE)
    holec = textc;
  else if (ex.add_hole == GROW_HOLE)
    holec = al_map_rgb(200, 0, 0);
  else
    holec = al_map_rgb(0, 200, 0);
  al_draw_textf(f, holec, textx, texty, 0,
                "Add Hole (%d) (H)", ex.cur_polygon);
  texty += texth;
}

/* Print vertices in a format for the test suite. */
static void print_vertices(void)
{
  int i;

  for (i = 0; i < ex.vertex_count; i++)
  {
    log_printf("v%-2d= %.2f, %.2f\n",
               i, ex.vertices[i].x, ex.vertices[i].y);
  }
  log_printf("\n");
}

int handle_event(ALLEGRO_EVENT *event)
{
  if (event->type == ALLEGRO_EVENT_DISPLAY_CLOSE)
  {
    return 1;
  }
  if (event->type == ALLEGRO_EVENT_KEY_CHAR)
  {
    if (event->keyboard.keycode == ALLEGRO_KEY_ESCAPE)
    {
      return 1;
    }
    switch (toupper(event->keyboard.unichar))
    {
    case ' ':
      if (++ex.mode >= MODE_MAX)
        ex.mode = 0;
      break;
    case 'J':
      if (++ex.join_style > ALLEGRO_LINE_JOIN_MITER)
        ex.join_style = ALLEGRO_LINE_JOIN_NONE;
      break;
    case 'C':
      if (++ex.cap_style > ALLEGRO_LINE_CAP_CLOSED)
        ex.cap_style = ALLEGRO_LINE_CAP_NONE;
      break;
    case '+':
      ex.thickness += 0.25f;
      break;
    case '-':
      ex.thickness -= 0.25f;
      if (ex.thickness <= 0.0f)
        ex.thickness = 0.0f;
      break;
    case '[':
      ex.miter_limit -= 0.1f;
      if (ex.miter_limit < 0.0f)
        ex.miter_limit = 0.0f;
      break;
    case ']':
      ex.miter_limit += 0.1f;
      if (ex.miter_limit >= 10.0f)
        ex.miter_limit = 10.0f;
      break;
    case 'S':
      ex.software = !ex.software;
      break;
    case 'R':
      reset();
      break;
    case 'P':
      print_vertices();
      break;
    case 'H':
      ex.add_hole = NEW_HOLE;
      break;
    }
  }
  if (event->type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN)
  {
    float x = event->mouse.x, y = event->mouse.y;
    transform(&x, &y);
    if (event->mouse.button == 1)
      lclick(x, y);
    if (event->mouse.button == 2)
      rclick(x, y);
    if (event->mouse.button == 3)
      ex.mdown = true;
  }
  if (event->type == ALLEGRO_EVENT_MOUSE_BUTTON_UP)
  {
    ex.cur_vertex = -1;
    if (event->mouse.button == 3)
      ex.mdown = false;
  }
  if (event->type == ALLEGRO_EVENT_MOUSE_AXES)
  {
    float x = event->mouse.x, y = event->mouse.y;
    transform(&x, &y);

    if (ex.mdown)
      scroll(event->mouse.dx, event->mouse.dy);
    else
      drag(x, y);

    ex.zoom *= pow(0.9, event->mouse.dz);
  }

  return 0;
}

void draw()
{
  if (ex.software)
  {
    al_set_target_bitmap(ex.dbuf);
    draw_all();
    al_set_target_backbuffer(ex.display);
    al_draw_bitmap(ex.dbuf, 0, 0, 0);
  }
  else
  {
    al_set_target_backbuffer(ex.display);
    draw_all();
  }
  al_flip_display();
}

#ifdef __EMSCRIPTEN__
void emscripten_main_loop()
{
  ALLEGRO_EVENT event;

  while (al_get_next_event(ex.queue, &event))
  {
    if (handle_event(&event) == 1)
    {
      emscripten_cancel_main_loop();
    }
  }
  draw();
}
#endif

void main_loop()
{
  ALLEGRO_EVENT event;

  while (true)
  {
    if (al_is_event_queue_empty(ex.queue))
      draw();
    al_wait_for_event(ex.queue, &event);
    if (handle_event(&event) == 1)
      break;
  }
}

int main(int argc, char **argv)
{
  // return 0;

  bool have_touch_input;

  (void)argc;
  (void)argv;

  // https://github.com/emscripten-core/emscripten/issues/7684#issuecomment-448446674
  // EmscriptenWebGLContextAttributes attr;
  // emscripten_webgl_init_context_attributes(&attr);
  // attr.majorVersion = 2;
  // attr.minorVersion = 0;
  // EMSCRIPTEN_WEBGL_CONTEXT_HANDLE ctx = emscripten_webgl_create_context(0, &attr);
  // emscripten_webgl_make_context_current(ctx);

  if (!al_init())
  {
    abort_example("Could not init Allegro.\n");
  }

  // Do a bunch of bogus allegro 4 stuff, just to confirm allegro-legacy linked correctly.
  {
    allegro_init();
    packfile_password("");
    int errrr = 0;
    if (file_exists("", 0, &errrr)) {
      log_printf("yes\n");
    } else {
      log_printf("no\n");
    }
    if (file_exists("zc.cfg", 0, &errrr)) {
      log_printf("yes\n");
    } else {
      log_printf("no\n");
    }
    // all_render_screen();
  }

  open_log();

  if (!al_init_primitives_addon())
  {
    abort_example("Could not init primitives.\n");
  }
  al_init_font_addon();
  al_install_mouse();
  al_install_keyboard();
  have_touch_input = al_install_touch_input();

  ex.display = al_create_display(800, 600);
  if (!ex.display)
  {
    abort_example("Error creating display\n");
  }

  ex.font = al_create_builtin_font();
  if (!ex.font)
  {
    abort_example("Error creating builtin font\n");
  }

  al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);
  ex.dbuf = al_create_bitmap(800, 600);
  ex.fontbmp = al_create_builtin_font();
  if (!ex.fontbmp)
  {
    abort_example("Error creating builtin font\n");
  }

  ex.queue = al_create_event_queue();
  al_register_event_source(ex.queue, al_get_keyboard_event_source());
  al_register_event_source(ex.queue, al_get_mouse_event_source());
  al_register_event_source(ex.queue, al_get_display_event_source(ex.display));
  if (have_touch_input)
  {
    al_register_event_source(ex.queue, al_get_touch_input_event_source());
    // al_register_event_source(ex.queue, al_get_touch_input_mouse_emulation_event_source());
  }

  ex.bg = al_map_rgba_f(1, 1, 0.9, 1);
  ex.fg = al_map_rgba_f(0, 0.5, 1, 1);

  reset();

  // all_render_screen();

#ifdef __EMSCRIPTEN__
  emscripten_set_main_loop(emscripten_main_loop, 0, true);
#else
  main_loop();
#endif

  al_destroy_display(ex.display);
  close_log(true);

  return 0;
}

// This is an allegro4 thing, but currently the allegro5 main handling is disabled (deferring to a4).
END_OF_MAIN()

/* vim: set sts=3 sw=3 et: */
