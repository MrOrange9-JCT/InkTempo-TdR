#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino.h>
#include <vector>
#include <algorithm>
#include <ArduinoJson.h>
#include "secrets.h"
#include "tasks.h"



std::vector<NotionTask> tasks; // Llista de tasques obtingudes de Notion (un vector és un array dinàmic)


void connect_wifi() {
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println(" Done");
}

void query_notion() {
    HTTPClient http;
    String url = "https://api.notion.com/v1/data_sources/" + String(NOTION_DATA_SOURCE) + "/query";

    http.begin(url);
    http.addHeader("Authorization", String("Bearer ") + NOTION_TOKEN);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Notion-Version", "2026-03-11");

    int code = http.POST("{}");
    Serial.println("HTTP: " + String(code));
    if (code != 200) {
        Serial.println(http.getString());
        http.end();
        return;
    }

    String payload = http.getString();
    http.end();

    JsonDocument doc;
    deserializeJson(doc, payload);

    tasks.clear();

    for (int i = 0; i < (int)doc["results"].size(); i++) {
        JsonObject page = doc["results"][i]["properties"];

        NotionTask t;
        t.done = page["Fet"]["checkbox"].as<bool>();
        t.subject = page["Matèria"]["select"]["name"].as<String>();
        t.title = page["Tasca"]["title"][0]["plain_text"].as<String>();
        t.deadline = page["Data límit"]["date"]["start"].as<String>(); // Data en format ISO 8601 (YYYY-MM-DDTHH:MM:SSZ+Offset)
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