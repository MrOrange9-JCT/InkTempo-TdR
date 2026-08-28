#pragma once
#include <Arduino.h>

struct NotionTask {
    bool done;
    String subject;
    String title;
    String deadline;
};

void connect_wifi();
void query_notion();
NotionTask get_task(int index);
int get_task_count();