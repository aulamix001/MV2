#pragma once
#include <array>
#include <algorithm>
namespace Scale
{
struct Mark {float value,y;};
template <size_t N> inline float position(float value,const std::array<Mark,N>& marks)
{
    if(value<=marks.front().value)return marks.front().y;
    for(size_t i=1;i<N;++i)
        if(value<=marks[i].value)
        {
            const auto a=marks[i-1],b=marks[i];
            return a.y+(b.y-a.y)*(value-a.value)/(b.value-a.value);
        }
    return marks.back().y;
}
inline constexpr std::array<Mark,7> low {{{0,836},{3,735},{6,672},{12,582},{24,454},{36,343},{48,270}}};
inline constexpr std::array<Mark,7> high {{{-48,837},{-36,752},{-24,638},{-12,513},{-6,424},{-3,362},{0,270}}};
inline constexpr std::array<Mark,8> outputMeter {{{-60,934},{-48,837},{-36,752},{-24,638},{-12,513},{-6,424},{-3,362},{0,270}}};
inline constexpr std::array<Mark,6> outputFader {{{-48,934},{-36,752},{-24,638},{-12,513},{0,420},{18,270}}};
}
