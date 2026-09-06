#include "date_format.h"

#include <stdio.h>

static const char *CAT_WEEKDAYS[7] = { "dg.", "dl.", "dt.", "dc.", "dj.", "dv.", "ds." };
static const char *CAT_MONTHS[12] = {
    "gen.", "febr.", "març", "abr.", "maig", "juny",
    "jul.", "ag.", "set.", "oct.", "nov.", "des."
};

static int day_of_week(int year, int month, int day) { 
    // Convertir numero a dia de la setmana amb l'algoritme de Zeller
    static const int offsets[] = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 };
    if (month < 3) year -= 1;
    return (year + year / 4 - year / 100 + year / 400 + offsets[month - 1] + day) % 7;
}

void format_date_cat(char *buffer, size_t buffer_size, int year, int month, int day) {
    // Formatejar la data en català amb el format "dg. 1 gen." (dia de la setmana, dia del mes, mes)
    if (buffer_size == 0) return;

    if (month < 1 || month > 12 || day < 1 || day > 31) {
        buffer[0] = '\0';
        return;
    }

    int weekday = day_of_week(year, month, day);
    snprintf(buffer, buffer_size, "%s %d %s", CAT_WEEKDAYS[weekday], day, CAT_MONTHS[month - 1]);
}
