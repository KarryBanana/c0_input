#pragma once

#include "./token.h"
#include <vector>

struct ActionLayer{
    int layer;
    std::vector<Token> symbols;   
};

class Layer {
  public:
    void addLayer();
  public:
    std::vector<ActionLayer> action_layer;
};