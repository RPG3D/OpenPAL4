#include "pal4/comparison_helper_interpolation_trace.h"
#include <cassert>
#include <sstream>
#include <iostream>

int main() {
    using pal4::ComparisonHelperInterpolationTrace;
    const std::array<float,3> first{-1.f,2.f,3.f},second{4.f,5.f,6.f},result{7.f,8.f,9.f};
    ComparisonHelperInterpolationTrace filtered(true,2u,8u);
    // Ignored helpers consume no prefix capacity, including after it fills.
    filtered.Add(4u,9u,1.f,first,second,result);
    filtered.Add(4u,8u,1.f,first,second,result);
    filtered.Add(3u,8u,2.f,first,second,result);
    filtered.Add(1u,9u,1.f,first,second,result);
    filtered.Add(1u,8u,1.f,first,second,result);
    std::ostringstream out;filtered.Write(out);const auto text=out.str();
    assert(text.find("\"count\":2,\"dropped\":1,\"filter_id\":8")!=std::string::npos);
    assert(text.find("\"id\":9")==std::string::npos);
    assert(text.find("\"first_bits\":[3212836864,1073741824,1077936128]")!=std::string::npos);
    assert(text.find("\"stage\":4")<text.find("\"stage\":3"));
    ComparisonHelperInterpolationTrace all(true,2u);
    all.Add(1u,8u,1.f,first,second,result);all.Add(2u,9u,1.f,first,second,result);
    out.str("");out.clear();all.Write(out);
    assert(out.str().find("\"count\":2,\"dropped\":0,\"filter_id\":null")!=std::string::npos);
    assert(out.str().find("\"id\":9")!=std::string::npos);
    ComparisonHelperInterpolationTrace disabled(false,2u,8u);
    disabled.Add(1u,8u,1.f,first,second,result);out.str("");out.clear();disabled.Write(out);
    assert(out.str().find("\"enabled\":false,\"capacity\":0,\"count\":0,\"dropped\":0")!=std::string::npos);
    std::cout<<"PASS: helper filter, bounded prefix/drop reporting, raw bits and disabled observation\n";
}
