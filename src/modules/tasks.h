#pragma once
#include <Arduino.h>

struct NotionTask {
    bool done;
    String subject;
    String title;
    String deadline;
    String id;
};

void connect_wifi();
void query_notion();
NotionTask get_task(int index);
int get_task_count();
void mark_task_done(int index, bool done);