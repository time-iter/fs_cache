#pragma once

struct cache_log *generate_log(struct cache_entry *entry, int type);

int commit_log(struct cache_log *log);

void recovery();

void block_recovery(struct cache_entry *entry);
