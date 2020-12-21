cmake_minimum_required(VERSION 3.0.0)
project (Playground)
# ============================================================
# Playground
# ============================================================
# Big hack
add_executable(FluxPlayground Playground.cc )
target_link_libraries(FluxPlayground PUBLIC FluxEngine)