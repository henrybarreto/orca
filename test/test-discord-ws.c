#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>

#include "discord.h"
#include "discord-internal.h"

#include "cee-utils.h"

pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;
bool g_keep_spamming = true;

void on_ready(struct discord *client, const struct discord_user *me) {
  log_info("Succesfully connected to Discord as %s#%s!",
      me->username, me->discriminator);
}

void on_disconnect(
  struct discord *client,
  const struct discord_user *bot,
  const struct discord_message *msg)
{
  if (msg->author->bot) return;

  struct discord_create_message_params params = { .content = "Disconnecting ..." };
  discord_create_message(client, msg->channel_id, &params, NULL);

  discord_gateway_shutdown(&client->gw);
}

void on_spam(
  struct discord *client,
  const struct discord_user *bot,
  const struct discord_message *msg)
{
  if (msg->author->bot) return;

  pthread_mutex_lock(&g_lock);
  g_keep_spamming = true;
  pthread_mutex_unlock(&g_lock);

  char number[256];
  struct discord_create_message_params params={0};

  bool keep_alive = true;
  for (int i=0 ;; ++i) {
    pthread_mutex_lock(&g_lock);
    keep_alive = g_keep_spamming;
    pthread_mutex_unlock(&g_lock);
    if (!keep_alive) break;

    snprintf(number, sizeof(number), "%d", i);
    params.content = number;
    discord_create_message(client, msg->channel_id, &params, NULL);
  };
}

void on_stop(
  struct discord *client,
  const struct discord_user *bot,
  const struct discord_message *msg)
{
  if (msg->author->bot) return;

  pthread_mutex_lock(&g_lock);
  g_keep_spamming = false;
  pthread_mutex_unlock(&g_lock);
}

void on_force_error(
  struct discord *client,
  const struct discord_user *bot,
  const struct discord_message *msg)
{
  if (msg->author->bot) return;

  ORCAcode code = discord_delete_channel(client, 123, NULL);
  struct discord_create_message_params params = { 
    .content = (char *)discord_strerror(code, client) 
  };

  discord_create_message(client, msg->channel_id, &params, NULL);
}

enum discord_event_handling_mode 
on_any_event(
  struct discord *client,
  struct discord_user *bot,
  struct sized_buffer *event_data,
  enum discord_gateway_events event) 
{
  return DISCORD_EVENT_CHILD_THREAD;
}

int main(int argc, char *argv[])
{
  const char *config_file;
  if (argc > 1)
    config_file = argv[1];
  else
    config_file = "../config.json";

  discord_global_init();

  struct discord *client = discord_config_init(config_file);
  assert(NULL != client && "Couldn't initialize client");

  /* trigger event callbacks in a multi-threaded fashion */
  discord_set_event_handler(client, &on_any_event);

  discord_set_on_ready(client, &on_ready);
  discord_set_on_command(client, "disconnect", &on_disconnect);
  discord_set_on_command(client, "spam", &on_spam);
  discord_set_on_command(client, "stop", &on_stop);
  discord_set_on_command(client, "force_error", &on_force_error);

  discord_run(client);

  discord_cleanup(client);

  discord_global_cleanup();
}

