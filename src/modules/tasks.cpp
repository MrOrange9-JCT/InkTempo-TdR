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
    WiFi.setSleep(true);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        
        if (WiFi.status() == WL_CONNECT_FAILED || WiFi.status() == WL_NO_SSID_AVAIL) {
            Serial.println(" WiFi connection failed, error code: " + String(WiFi.status()));
            return;
        }
    }
    Serial.println(" Connected to WiFi");
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
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[Notion] WiFi not connected, cannot query Notion");
        NotionTask error_task = {false, "ERROR", "No hi ha connexió WiFi", "", "error_no_wifi"};
        tasks.clear();
        tasks.push_back(error_task);
        return;
    }

    HTTPClient http;
    String url = "https://api.notion.com/v1/data_sources/" + String(NOTION_DATA_SOURCE) + "/query";

    http.begin(url);
    http.setConnectTimeout(10000);
    http.setTimeout(15000);
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
    Serial.println("[Notion] Response bytes: " + String(payload.length()));
    if (payload.isEmpty()) {
        Serial.println("[Notion] Empty response body");
        http.end();
        return;
    }

    JsonDocument doc;
    DeserializationError parse_error = deserializeJson(doc, payload);
    if (parse_error) {
        Serial.println("[Notion] JSON parse error: " + String(parse_error.c_str()));
        http.end();
        return;
    }

    JsonArray results = doc["results"].as<JsonArray>();
    if (results.isNull()) {
        Serial.println("[Notion] Response has no results array");
        http.end();
        return;
    }

    tasks.clear();
    tasks.reserve(results.size());

    for (JsonObject page : results) {
        JsonObject properties = page["properties"].as<JsonObject>();
        JsonArray title = properties["Tasca"]["title"].as<JsonArray>();
        JsonObject subject = properties["Matèria"]["select"].as<JsonObject>();
        JsonObject deadline_date = properties["Data límit"]["date"].as<JsonObject>();
        if (properties.isNull() || title.isNull() || subject.isNull()) {
            Serial.println("[Notion] Skipping page with unexpected properties");
            continue;
        }

        NotionTask t;
        t.done = properties["Fet"]["checkbox"].as<bool>();
        t.subject = subject["name"].as<String>();
        t.title = title[0]["plain_text"].as<String>();
        if (!deadline_date.isNull() && !deadline_date["start"].isNull()) {
            t.deadline = deadline_date["start"].as<String>(); // Data en format ISO 8601 (YYYY-MM-DDTHH:MM:SSZ+Offset)
        }
        t.id = page["id"].as<String>(); // ID de la pàgina a Notion
        //TODO: Convertir la data a format més llegible (ex. dt. 8 set.)
        
        tasks.push_back(t);

        // Serial.println("--- Task " + String(i + 1) + " ---");
        // Serial.println("  Title:    " + t.title);
        // Serial.println("  Done:     " + String(t.done ? "Yes" : "No"));
        // Serial.println("  Subject:  " + t.subject);
        // Serial.println("  Deadline: " + t.deadline);
    }

    http.end();

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
        return NotionTask{false, "", "", "", ""};
    }
}

int get_task_count() {
    return (int)tasks.size();
}

void mark_task_done(int index, bool done) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[Notion] WiFi not connected, cannot update task");
        return;
    }
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