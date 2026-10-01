/*
 * CONFIGURACIÓ DEL PROJECTE "config.h"
 * Aquest arxiu pot ser modificat per l'usuari
 * Conté diverses configuracions i constants que es poden ajustar segons les preferències de l'usuari
 */

#pragma once

/*====================*
    POMODORO 
 *====================*/

#define POMODORO_FOCUS_MINUTES 25 // Durada del període de concentració (min)
#define POMODORO_SHORT_BREAK_MINUTES 5 // Durada del descans curt (min)
#define POMODORO_LONG_BREAK_MINUTES 10 // Durada del descans llarg (min)
#define POMODORO_CYCLES_BEFORE_LONG_BREAK 2 // Nombre de cicles (sessions de concentració) abans del descans llarg

/*====================*
    ESTADÍSTIQUES 
 *====================*/

#define STATS_FOCUS_GOAL_MINUTES 90 // Objectiu de temps de concentració diari (min)
#define STATS_TASKS_GOAL 5 // Objectiu de tasques completes diari

/*====================*
    CREDENCIALS 
 *====================*/

// Les claus i credencials es defineixen a l'arxiu src/secrets.h
// Es pot trobar un exemple a src/secrets.h.template