import 'package:flutter/material.dart';

class Combination {
  List<Color> colors = [Colors.white, Colors.red, Colors.green, Colors.blue, Colors.yellow, Colors.deepPurple];
  List<Color> slots = [];
  int cluesCorrect = 0;
  int cluesPresent = 0;

  Combination.fromRandom() {
    for (int i = 0; i < 4; i++) {
      slots.add(colors[0]);
    }
  }
  Combination.fromBuffer(List<int> values) {
    print(values.toString());
    for (int i = 0; i < 8; i += 2) {
      if (values[i + 1] == 0x01) {
        slots.add(colors[values[i]]);
      }
    }
    cluesPresent = values[8];
    cluesCorrect = values[9];
  }

  List<int> toBuffer() {
    List<int> list = [];
    for (Color color in slots) {
      list.add(colors.indexOf(color));
    }
    return list;
  }
}
