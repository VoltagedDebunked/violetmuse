#include <dirent.h>
#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <vlc/vlc.h>

libvlc_instance_t *vlc_instance;
libvlc_media_player_t *media_player;
libvlc_media_list_t *media_list;
libvlc_media_list_player_t *media_list_player;
int is_looping = 0;

GtkWidget *playlist_view;
GtkListStore *playlist_store;
GtkWidget *progress_bar;
GtkWidget *current_song_label;
GtkWidget *search_entry;

void
play_music (GtkButton *button, gpointer data)
{
  if (media_list_player)
    {
      libvlc_media_list_player_play (media_list_player);
    }
}

void
pause_music (GtkButton *button, gpointer data)
{
  if (media_list_player)
    {
      libvlc_media_list_player_pause (media_list_player);
    }
}

void
stop_music (GtkButton *button, gpointer data)
{
  if (media_list_player)
    {
      libvlc_media_list_player_stop (media_list_player);
      gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (progress_bar), 0.0);
      gtk_label_set_text (GTK_LABEL (current_song_label), "No song playing");
    }
}

void
update_progress_bar ()
{
  if (media_player)
    {
      int64_t pos = libvlc_media_player_get_time (media_player);
      int64_t duration = libvlc_media_player_get_length (media_player);
      if (duration > 0)
        {
          gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (progress_bar),
                                         (double)pos / duration);
        }
    }
}

gboolean
on_update_progress (gpointer data)
{
  update_progress_bar ();
  return G_SOURCE_CONTINUE;
}

void
load_music ()
{
  const char *home_dir = getenv ("HOME");
  char music_dir[256];
  snprintf (music_dir, sizeof (music_dir), "%s/Music", home_dir);

  DIR *dir = opendir (music_dir);
  if (!dir)
    return;

  struct dirent *entry;
  while ((entry = readdir (dir)) != NULL)
    {
      if (strstr (entry->d_name, ".mp3") || strstr (entry->d_name, ".wav"))
        {
          char filepath[512];
          snprintf (filepath, sizeof (filepath), "%s/%s", music_dir,
                    entry->d_name);
          libvlc_media_t *media
              = libvlc_media_new_path (vlc_instance, filepath);
          libvlc_media_list_add_media (media_list, media);
          libvlc_media_release (media);

          GtkTreeIter iter;
          gchar *name = g_path_get_basename (entry->d_name);
          gchar *title
              = g_strndup (name, strlen (name) - strlen (strrchr (name, '.')));
          gtk_list_store_append (playlist_store, &iter);
          gtk_list_store_set (playlist_store, &iter, 0, title, 1,
                              "Artist Name", -1);
          g_free (title);
          g_free (name);
        }
    }
  closedir (dir);
}

void
toggle_loop (GtkButton *button, gpointer data)
{
  is_looping = !is_looping;
  libvlc_media_list_player_set_playback_mode (
      media_list_player,
      is_looping ? libvlc_playback_mode_loop : libvlc_playback_mode_default);
}

void
on_home_clicked (GtkButton *button, gpointer data)
{
  g_print ("Home button clicked\n");
}

void
on_search_clicked (GtkButton *button, gpointer data)
{
  const gchar *search_text = gtk_entry_get_text (GTK_ENTRY (search_entry));
  GtkTreeIter iter;
  gboolean valid
      = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (playlist_store), &iter);
  while (valid)
    {
      gchar *title;
      gtk_tree_model_get (GTK_TREE_MODEL (playlist_store), &iter, 0, &title,
                          -1);
      if (g_str_has_prefix (title, search_text))
        {
          gtk_tree_selection_select_iter (
              gtk_tree_view_get_selection (GTK_TREE_VIEW (playlist_view)),
              &iter);
          g_free (title);
          break;
        }
      g_free (title);
      valid
          = gtk_tree_model_iter_next (GTK_TREE_MODEL (playlist_store), &iter);
    }
}

void
on_playlist_row_activated (GtkTreeView *tree_view, GtkTreePath *path,
                           GtkTreeViewColumn *column, gpointer user_data)
{
  GtkTreeIter iter;
  gtk_tree_model_get_iter (GTK_TREE_MODEL (playlist_store), &iter, path);
  gchar *title;
  gtk_tree_model_get (GTK_TREE_MODEL (playlist_store), &iter, 0, &title, -1);
  gtk_label_set_text (GTK_LABEL (current_song_label), title);
  int index = gtk_tree_path_get_indices (path)[0];
  libvlc_media_list_player_play_item_at_index (media_list_player, index);
  g_free (title);
}

void
on_volume_changed (GtkRange *range, gpointer data)
{
  gdouble volume = gtk_range_get_value (range);
  libvlc_audio_set_volume (media_player, (int)volume);
}

#define GTK_POSITION_RIGHT 2

int
main (int argc, char *argv[])
{
  gtk_init (&argc, &argv);

  vlc_instance = libvlc_new (0, NULL);
  media_player = libvlc_media_player_new (vlc_instance);
  media_list = libvlc_media_list_new (vlc_instance);
  media_list_player = libvlc_media_list_player_new (vlc_instance);
  libvlc_media_list_player_set_media_player (media_list_player, media_player);
  libvlc_media_list_player_set_media_list (media_list_player, media_list);

  GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), "VioletMuse");
  gtk_window_set_default_size (GTK_WINDOW (window), 600, 320);
  gtk_container_set_border_width (GTK_CONTAINER (window), 10);
  g_signal_connect (window, "destroy", G_CALLBACK (gtk_main_quit), NULL);

  GtkWidget *main_container = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  GtkWidget *header_bar = gtk_header_bar_new ();
  gtk_header_bar_set_title (GTK_HEADER_BAR (header_bar), "VioletMuse");
  gtk_header_bar_set_show_close_button (GTK_HEADER_BAR (header_bar), TRUE);
  gtk_window_set_titlebar (GTK_WINDOW (window), header_bar);

  GtkWidget *sidebar = gtk_box_new (GTK_ORIENTATION_VERTICAL, 5);
  GtkWidget *home_button = gtk_button_new_with_label ("Home");
  search_entry = gtk_entry_new ();
  GtkWidget *search_button = gtk_button_new_with_label ("Search");

  g_signal_connect (home_button, "clicked", G_CALLBACK (on_home_clicked),
                    NULL);
  g_signal_connect (search_button, "clicked", G_CALLBACK (on_search_clicked),
                    NULL);

  gtk_box_pack_start (GTK_BOX (sidebar), home_button, FALSE, FALSE, 5);
  gtk_box_pack_start (GTK_BOX (sidebar), search_entry, FALSE, FALSE, 5);
  gtk_box_pack_start (GTK_BOX (sidebar), search_button, FALSE, FALSE, 5);

  playlist_store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_STRING);
  playlist_view
      = gtk_tree_view_new_with_model (GTK_TREE_MODEL (playlist_store));
  GtkCellRenderer *renderer_title = gtk_cell_renderer_text_new ();
  GtkCellRenderer *renderer_artist = gtk_cell_renderer_text_new ();

  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (playlist_view),
                                               -1, "Title", renderer_title,
                                               "text", 0, NULL);
  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (playlist_view),
                                               -1, "Artist", renderer_artist,
                                               "text", 1, NULL);

  g_signal_connect (playlist_view, "row-activated",
                    G_CALLBACK (on_playlist_row_activated), NULL);

  progress_bar = gtk_progress_bar_new ();
  current_song_label = gtk_label_new ("No song playing");

  GtkWidget *playback_bar = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
  GtkWidget *play_button = gtk_button_new_with_label ("Play");
  GtkWidget *pause_button = gtk_button_new_with_label ("Pause");
  GtkWidget *stop_button = gtk_button_new_with_label ("Stop");
  GtkWidget *loop_button = gtk_button_new_with_label ("Loop");
  GtkWidget *volume_scale
      = gtk_scale_new_with_range (GTK_ORIENTATION_HORIZONTAL, 0, 100, 1);
  gtk_scale_set_value_pos (GTK_SCALE (volume_scale), GTK_POSITION_RIGHT);
  gtk_range_set_value (GTK_RANGE (volume_scale), 100);

  g_signal_connect (play_button, "clicked", G_CALLBACK (play_music), NULL);
  g_signal_connect (pause_button, "clicked", G_CALLBACK (pause_music), NULL);
  g_signal_connect (stop_button, "clicked", G_CALLBACK (stop_music), NULL);
  g_signal_connect (loop_button, "clicked", G_CALLBACK (toggle_loop), NULL);
  g_signal_connect (volume_scale, "value-changed",
                    G_CALLBACK (on_volume_changed), NULL);

  gtk_box_pack_start (GTK_BOX (playback_bar), play_button, FALSE, FALSE, 5);
  gtk_box_pack_start (GTK_BOX (playback_bar), pause_button, FALSE, FALSE, 5);
  gtk_box_pack_start (GTK_BOX (playback_bar), stop_button, FALSE, FALSE, 5);
  gtk_box_pack_start (GTK_BOX (playback_bar), loop_button, FALSE, FALSE, 5);
  gtk_box_pack_start (GTK_BOX (playback_bar), volume_scale, TRUE, TRUE, 5);

  GtkWidget *content_area = gtk_box_new (GTK_ORIENTATION_VERTICAL, 5);
  gtk_box_pack_start (GTK_BOX (content_area), playlist_view, TRUE, TRUE, 5);
  gtk_box_pack_start (GTK_BOX (content_area), progress_bar, FALSE, FALSE, 5);
  gtk_box_pack_start (GTK_BOX (content_area), current_song_label, FALSE, FALSE,
                      5);
  gtk_box_pack_start (GTK_BOX (content_area), playback_bar, FALSE, FALSE, 5);

  GtkWidget *main_layout = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_pack_start (GTK_BOX (main_layout), sidebar, FALSE, FALSE, 5);
  gtk_box_pack_start (GTK_BOX (main_layout), content_area, TRUE, TRUE, 5);

  gtk_container_add (GTK_CONTAINER (main_container), main_layout);
  gtk_container_add (GTK_CONTAINER (window), main_container);

  load_music ();

  g_timeout_add (1000, on_update_progress, NULL);
  gtk_widget_show_all (window);
  gtk_main ();

  libvlc_media_list_player_release (media_list_player);
  libvlc_media_list_release (media_list);
  libvlc_media_player_release (media_player);
  libvlc_release (vlc_instance);
  return 0;
}