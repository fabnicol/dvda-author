#ifndef JOBS_HPP
#define JOBS_HPP


namespace jobs
{
static    int notrim = 0x01;
static    int seamless = 0x02;
static    int discrete = 0x04;
static    int padded = 0x08;
static    int autoSet = 0xF0;
static    int continuous = seamless | padded;
static    int backward = 0x10;
static    int nearest = 0x20;
static    int forward = 0x40;
}

#endif // JOBS_HPP
