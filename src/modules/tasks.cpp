#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino.h>
#include <vector>
#include <algorithm>
#include <ArduinoJson.h>
#include "secrets.h"
#include "tasks.h"
#include "stats.h"
#include "date_format.h"

std::vector<NotionTask> tasks; // Llista de tasques obtingudes de Notion (un vector és un array dinàmic)


void connect_wifi() {
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("[Notion] Connecting WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println(" Done");
}

String format_deadline_cat(const String &iso) {
    if (iso.isEmpty()) return "";

    int year, month, day, hour, minute;
    // Parses "2026-03-02T12:45:00.000+01:00" — ignores seconds/ms/offset
    int parsed = sscanf(iso.c_str(), "%d-%d-%dT%d:%d", &year, &month, &day, &hour, &minute);
    if (parsed < 3) {
        parsed = sscanf(iso.c_str(), "%d-%d-%d", &year, &month, &day);
    }
    if (parsed < 3) return iso; // fallback a string original

    char buf[32];
    char date_buf[24];
    format_date_cat(date_buf, sizeof(date_buf), year, month, day);

    if (parsed >= 5) {
        // Data i hora
        snprintf(buf, sizeof(buf), "%s %02d:%02d", date_buf, hour, minute);
    } else {
        // Només data
        snprintf(buf, sizeof(buf), "%s", date_buf);
    }

    return String(buf);
}

void query_notion() {
    HTTPClient http;
    String url = "https://api.notion.com/v1/data_sources/" + String(NOTION_DATA_SOURCE) + "/query";

    http.begin(url);
    http.addHeader("Authorization", String("Bearer ") + NOTION_TOKEN);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Notion-Version", "2026-03-11");

    int code = http.POST("{}");
    Serial.println("[Notion] HTTP: " + String(code));
    if (code != 200) {
        Serial.println("[Notion] Error: " + http.getString());
        http.end();
        return;
    }

    String payload = http.getString();
    http.end();

    JsonDocument doc;
    deserializeJson(doc, payload);

    tasks.clear();

    for (int i = 0; i < (int)doc["results"].size(); i++) {
        JsonObject page = doc["results"][i];

        NotionTask t;
        t.done = page["properties"]["Fet"]["checkbox"].as<bool>();
        t.subject = page["properties"]["Matèria"]["select"]["name"].as<String>();
        t.title = page["properties"]["Tasca"]["title"][0]["plain_text"].as<String>();
        t.deadline = page["properties"]["Data límit"]["date"]["start"].as<String>(); // Data en format ISO 8601 (YYYY-MM-DDTHH:MM:SSZ+Offset)
        t.id = page["id"].as<String>(); // ID de la pàgina a Notion
        //TODO: Convertir la data a format més llegible (ex. dt. 8 set.)
        
        tasks.push_back(t);

        // Serial.println("--- Task " + String(i + 1) + " ---");
        // Serial.println("  Title:    " + t.title);
        // Serial.println("  Done:     " + String(t.done ? "Yes" : "No"));
        // Serial.println("  Subject:  " + t.subject);
        // Serial.println("  Deadline: " + t.deadline);
    }

    // Primer les tasques pendents i després les fetes; dins de cada grup, per data límit ascendent
    std::sort(tasks.begin(), tasks.end(), [](const NotionTask &a, const NotionTask &b) {
        if (a.done != b.done) return !a.done;
        if (a.deadline.isEmpty() && b.deadline.isEmpty()) return false;
        if (a.deadline.isEmpty()) return false; // a goes after b
        if (b.deadline.isEmpty()) return true;  // a goes before b
        return a.deadline < b.deadline;
    });

    for (NotionTask &task : tasks) {
        task.deadline = format_deadline_cat(task.deadline);
    }
}

NotionTask get_task(int index) {
    if (index < tasks.size()) {
        return tasks[index];
    } else {
        // Retorna una tasca buida si l'índex és fora de rang
        return NotionTask{false, "", "", ""};
    }
}

int get_task_count() {
    return (int)tasks.size();
}

void mark_task_done(int index, bool done) {
    if (index < tasks.size()) {
        tasks[index].done = done;
        stats_add_task_completed(done); // Incrementa el comptador de tasques completades només si s'ha marcat com a fet

        HTTPClient http;
        String url = "https://api.notion.com/v1/pages/" + tasks[index].id;

        http.begin(url);
        http.addHeader("Authorization", String("Bearer ") + NOTION_TOKEN);
        http.addHeader("Content-Type", "application/json");
        http.addHeader("Notion-Version", "2026-03-11");

        String payload = "{\"properties\": {\"Fet\": {\"checkbox\": " + String(done ? "true" : "false") + "}}}";

        int response_code = http.PATCH(payload);
        Serial.println("[Notion] HTTP: " + String(response_code));
        if (response_code != 200) {
            Serial.println("[Notion] Error: " + http.getString());
        }
        http.end();
    }
}