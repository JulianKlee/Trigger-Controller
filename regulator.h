#pragma once
#ifndef REGULATOR_H
#define REGULATOR_H

int regulator_start(void);
int regulator_target(float amp, float pha);

#endif/*REGULATOR_H*/
