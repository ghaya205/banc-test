#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// --- Constants ---
#define UI_FILE "pathfinder_ui.ui"
#define LEVEL_FILE "level_data.txt"
#define SCORE_FILE "high_score.txt"
#define MAX_LEVELS 5
#define MAX_GRID_SIZE 20

// --- Structures ---
typedef enum
{
  GAME_STATE_RUNNING,
  GAME_STATE_WIN_LEVEL,
  GAME_STATE_GAME_OVER
} GameState;

typedef struct
{
  char grid[MAX_GRID_SIZE][MAX_GRID_SIZE];
  int rows;
  int cols;
  double time_limit; // seconds
} Level;

typedef struct
{
  Level levels[MAX_LEVELS];
  int current_level_idx;
  GameState state;
  double player_size;

  int player_grid_x;
  int player_grid_y;

  gint64 start_time_us;
  double elapsed_time;
  int score;
  int high_score;

  gboolean is_w_pressed;
  gboolean is_s_pressed;
  gboolean is_a_pressed;
  gboolean is_d_pressed;

  GtkWidget *window;
  GtkWidget *drawing_area;
  GtkWidget *status_label;
  GtkWidget *message_label;
} GameData;

// Global
static GameData game_data;

// --- Forward declarations ---
static gboolean load_all_levels(void);
static void start_level(int level_idx);
static void game_over(const char *reason);
static void update_status_labels(void);
static void load_css(void);

// --- High score ---
static void load_high_score(void)
{
  FILE *file = fopen(SCORE_FILE, "r");
  if (file)
  {
    if (fscanf(file, "%d", &game_data.high_score) != 1)
      game_data.high_score = 0;
    fclose(file);
  }
  else
  {
    game_data.high_score = 0;
  }
}

static void save_high_score(void)
{
  if (game_data.score > game_data.high_score)
    game_data.high_score = game_data.score;

  FILE *file = fopen(SCORE_FILE, "w");
  if (file)
  {
    fprintf(file, "%d", game_data.high_score);
    fclose(file);
  }
}

// --- Level loading ---
static gboolean load_all_levels(void)
{
  FILE *file = fopen(LEVEL_FILE, "r");
  if (!file)
  {
    g_printerr("Error: Could not open level file %s\n", LEVEL_FILE);
    return FALSE;
  }

  char line[1024];
  int current_level = -1;
  int row_count = 0;

  while (fgets(line, sizeof(line), file))
  {
    g_strstrip(line);
    if (line[0] == '\0')
      continue;

    // Detect level header
    if (strstr(line, "--- Level"))
    {
      current_level++;
      if (current_level >= MAX_LEVELS)
        break;
      row_count = 0;

      // Next line should contain size and time
      if (!fgets(line, sizeof(line), file))
      {
        g_printerr("Error: Missing size/time for level %d\n", current_level + 1);
        fclose(file);
        return FALSE;
      }
      g_strstrip(line);

      int r, c;
      double t;
      if (sscanf(line, " %d , %d , %lf", &r, &c, &t) != 3)
      {
        g_printerr("Error: Invalid size/time for level %d: '%s'\n", current_level + 1, line);
        fclose(file);
        return FALSE;
      }
      Level *level = &game_data.levels[current_level];
      level->rows = r;
      level->cols = c;
      level->time_limit = t;

      continue;
    }

    // Grid lines
    if (current_level >= 0 && current_level < MAX_LEVELS)
    {
      Level *level = &game_data.levels[current_level];
      if (row_count < level->rows)
      {
        for (int j = 0; j < level->cols; j++)
          level->grid[row_count][j] = (line[j] != '\0') ? line[j] : '.';
        row_count++;
      }
    }
  }

  fclose(file);
  g_print("Levels loaded successfully (%d levels).\n", MAX_LEVELS);
  return TRUE;
}

// --- Start level ---
static void start_level(int level_idx)
{
  if (level_idx >= MAX_LEVELS)
  {
    game_data.state = GAME_STATE_WIN_LEVEL;
    gtk_label_set_text(GTK_LABEL(game_data.message_label),
                       "CONGRATULATIONS! You beat all levels!");
    save_high_score();
    return;
  }

  Level *level = &game_data.levels[level_idx];
  gboolean start_found = FALSE;
  for (int i = 0; i < level->rows; i++)
    for (int j = 0; j < level->cols; j++)
      if (level->grid[i][j] == 'S')
      {
        game_data.player_grid_y = i;
        game_data.player_grid_x = j;
        start_found = TRUE;
        break;
      }
  if (!start_found)
  {
    game_over("Level data is missing a start point (S)!");
    return;
  }

  game_data.current_level_idx = level_idx;
  game_data.state = GAME_STATE_RUNNING;
  game_data.elapsed_time = 0.0;
  game_data.is_w_pressed = game_data.is_s_pressed =
      game_data.is_a_pressed = game_data.is_d_pressed = FALSE;
  game_data.start_time_us = g_get_monotonic_time();

  char msg[128];
  snprintf(msg, sizeof(msg), "Level %d started! Use WASD to reach 'E'.", level_idx + 1);
  gtk_label_set_text(GTK_LABEL(game_data.message_label), msg);
  update_status_labels();
}

// --- Game over ---
static void game_over(const char *reason)
{
  game_data.state = GAME_STATE_GAME_OVER;
  save_high_score();
  char msg[128];
  snprintf(msg, sizeof(msg), "GAME OVER! %s | Press SPACE to restart Level 1.", reason);
  gtk_label_set_text(GTK_LABEL(game_data.message_label), msg);
  update_status_labels();
  gtk_widget_queue_draw(game_data.drawing_area);
}

// --- Draw callback ---
static void draw_cb(GtkDrawingArea *area, cairo_t *cr, int width, int height, gpointer user_data)
{
  Level *level = &game_data.levels[game_data.current_level_idx];
  if (level->rows == 0 || level->cols == 0)
    return;

  double cell_w = (double)width / level->cols;
  double cell_h = (double)height / level->rows;
  double player_r = MIN(cell_w, cell_h) * 0.4;
  game_data.player_size = player_r * 2.0;

  for (int i = 0; i < level->rows; i++)
    for (int j = 0; j < level->cols; j++)
    {
      cairo_save(cr);
      cairo_rectangle(cr, j * cell_w, i * cell_h, cell_w, cell_h);

      switch (level->grid[i][j])
      {
      case '#':
        cairo_set_source_rgb(cr, 0.2, 0.2, 0.2);
        break;
      case 'S':
        cairo_set_source_rgb(cr, 0.0, 0.4, 0.0);
        break;
      case 'E':
        cairo_set_source_rgb(cr, 0.8, 0.0, 0.0);
        break;
      default:
        cairo_set_source_rgb(cr, 0.1, 0.1, 0.1);
        break;
      }
      cairo_fill_preserve(cr);
      cairo_set_source_rgb(cr, 0.3, 0.3, 0.3);
      cairo_set_line_width(cr, 0.5);
      cairo_stroke(cr);
      cairo_restore(cr);
    }

  if (game_data.state == GAME_STATE_RUNNING || game_data.state == GAME_STATE_WIN_LEVEL)
  {
    double px = game_data.player_grid_x * cell_w + cell_w / 2.0;
    double py = game_data.player_grid_y * cell_h + cell_h / 2.0;
    cairo_set_source_rgb(cr, 0.0, 0.7, 1.0);
    cairo_arc(cr, px, py, player_r, 0, 2 * M_PI);
    cairo_fill(cr);
  }
}

// --- Update status labels ---
static void update_status_labels(void)
{
  char str[128];
  double time_remaining = game_data.levels[game_data.current_level_idx].time_limit - game_data.elapsed_time;

  if (game_data.state == GAME_STATE_GAME_OVER)
  {
    snprintf(str, sizeof(str),
             "Level %d Failed | Score: %d | High Score: %d",
             game_data.current_level_idx + 1, game_data.score, game_data.high_score);
  }
  else
  {
    snprintf(str, sizeof(str),
             "Level %d/%d | Time: %.1fs%s | Score: %d | High Score: %d",
             game_data.current_level_idx + 1, MAX_LEVELS,
             time_remaining, (time_remaining < 0.0 ? " (Over)" : "s"),
             game_data.score, game_data.high_score);
  }
  gtk_label_set_text(GTK_LABEL(game_data.status_label), str);
}

// --- Tick callback ---
static gboolean tick_cb(GtkWidget *widget, GdkFrameClock *frame_clock, gpointer user_data)
{
  gint64 now_us = g_get_monotonic_time();
  gdouble dt = (now_us - game_data.start_time_us) / 1000000.0;
  game_data.start_time_us = now_us;
  static gdouble acc = 0.0;
  acc += dt;
  const gdouble FIXED_STEP = 0.05;

  if (game_data.state == GAME_STATE_RUNNING)
  {
    game_data.elapsed_time += dt;
    if (game_data.elapsed_time >= game_data.levels[game_data.current_level_idx].time_limit)
    {
      game_over("Time limit exceeded!");
      goto redraw;
    }

    while (acc >= FIXED_STEP)
    {
      acc -= FIXED_STEP;
      int nx = game_data.player_grid_x;
      int ny = game_data.player_grid_y;
      gboolean moved = FALSE;

      if (game_data.is_w_pressed)
      {
        ny--;
        moved = TRUE;
      }
      else if (game_data.is_s_pressed)
      {
        ny++;
        moved = TRUE;
      }
      else if (game_data.is_a_pressed)
      {
        nx--;
        moved = TRUE;
      }
      else if (game_data.is_d_pressed)
      {
        nx++;
        moved = TRUE;
      }

      if (moved)
      {
        Level *lvl = &game_data.levels[game_data.current_level_idx];
        if (nx >= 0 && nx < lvl->cols && ny >= 0 && ny < lvl->rows)
        {
          char target = lvl->grid[ny][nx];
          if (target != '#')
          {
            game_data.player_grid_x = nx;
            game_data.player_grid_y = ny;

            if (target == 'E')
            {
              game_data.score += 100;
              game_data.score += (int)((lvl->time_limit - game_data.elapsed_time) * 10);
              game_data.state = GAME_STATE_WIN_LEVEL;
              char msg[128];
              snprintf(msg, sizeof(msg),
                       "LEVEL %d COMPLETE! Score: %d | Press SPACE for next level.",
                       game_data.current_level_idx + 1, game_data.score);
              gtk_label_set_text(GTK_LABEL(game_data.message_label), msg);
            }
          }
        }
      }
    }
  }

redraw:
  update_status_labels();
  gtk_widget_queue_draw(widget);
  return G_SOURCE_CONTINUE;
}

// --- Input ---
static gboolean key_pressed_cb(GtkEventControllerKey *c, guint keyval, guint keycode, GdkModifierType state, gpointer user_data)
{
  if (game_data.state != GAME_STATE_RUNNING)
  {
    if (keyval == GDK_KEY_space || keyval == GDK_KEY_Return)
    {
      if (game_data.state == GAME_STATE_GAME_OVER)
      {
        game_data.score = 0;
        start_level(0);
      }
      else if (game_data.state == GAME_STATE_WIN_LEVEL)
      {
        start_level(game_data.current_level_idx + 1);
      }
    }
    return G_SOURCE_CONTINUE;
  }

  game_data.is_w_pressed = game_data.is_s_pressed = game_data.is_a_pressed = game_data.is_d_pressed = FALSE;
  switch (keyval)
  {
  case GDK_KEY_W:
  case GDK_KEY_w:
    game_data.is_w_pressed = TRUE;
    break;
  case GDK_KEY_S:
  case GDK_KEY_s:
    game_data.is_s_pressed = TRUE;
    break;
  case GDK_KEY_A:
  case GDK_KEY_a:
    game_data.is_a_pressed = TRUE;
    break;
  case GDK_KEY_D:
  case GDK_KEY_d:
    game_data.is_d_pressed = TRUE;
    break;
  }
  return G_SOURCE_CONTINUE;
}

static gboolean key_released_cb(GtkEventControllerKey *c, guint keyval, guint keycode, GdkModifierType state, gpointer user_data)
{
  switch (keyval)
  {
  case GDK_KEY_W:
  case GDK_KEY_w:
    game_data.is_w_pressed = FALSE;
    break;
  case GDK_KEY_S:
  case GDK_KEY_s:
    game_data.is_s_pressed = FALSE;
    break;
  case GDK_KEY_A:
  case GDK_KEY_a:
    game_data.is_a_pressed = FALSE;
    break;
  case GDK_KEY_D:
  case GDK_KEY_d:
    game_data.is_d_pressed = FALSE;
    break;
  }
  return G_SOURCE_CONTINUE;
}

// --- CSS ---
static void load_css(void)
{
  GtkCssProvider *provider = gtk_css_provider_new();
  GdkDisplay *display = gdk_display_get_default();
  const char *css =
      ".game-area { background-color: #1e1e1e; }"
      "label.title-4 { font-size: 20px; font-weight: bold; color: #eeeeee; }"
      "label.subtitle { font-size: 16px; color: #999999; }";
  gtk_css_provider_load_from_string(provider, css);
  gtk_style_context_add_provider_for_display(display, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  g_object_unref(provider);
}

// --- Activate ---
static void activate(GtkApplication *app, gpointer user_data)
{
  load_css();
  GtkBuilder *builder = gtk_builder_new_from_file(UI_FILE);
  if (!builder)
  {
    g_printerr("Error: Could not load UI file\n");
    return;
  }

  game_data.window = GTK_WIDGET(gtk_builder_get_object(builder, "app_window"));
  gtk_application_add_window(app, GTK_WINDOW(game_data.window));

  game_data.drawing_area = GTK_WIDGET(gtk_builder_get_object(builder, "game_area"));
  game_data.status_label = GTK_WIDGET(gtk_builder_get_object(builder, "status_label"));
  game_data.message_label = GTK_WIDGET(gtk_builder_get_object(builder, "message_label"));

  gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(game_data.drawing_area), draw_cb, NULL, NULL);

  GtkEventController *key_ctrl = gtk_event_controller_key_new();
  g_signal_connect(key_ctrl, "key-pressed", G_CALLBACK(key_pressed_cb), NULL);
  g_signal_connect(key_ctrl, "key-released", G_CALLBACK(key_released_cb), NULL);
  gtk_widget_add_controller(game_data.window, key_ctrl);

  gtk_widget_add_tick_callback(game_data.drawing_area, tick_cb, NULL, NULL);

  g_object_unref(builder);

  load_high_score();
  start_level(0);

  gtk_window_present(GTK_WINDOW(game_data.window));
}

// --- Main ---
int main(int argc, char **argv)
{
  if (!load_all_levels())
  {
    g_printerr("Fatal Error: Could not load levels.\n");
    return 1;
  }

  GtkApplication *app = gtk_application_new("org.gtk.pathfinder", G_APPLICATION_DEFAULT_FLAGS);
  g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
  int status = g_application_run(G_APPLICATION(app), argc, argv);
  g_object_unref(app);
  return status;
}
