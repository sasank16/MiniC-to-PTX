int demo_opt(int a, int b) {
    // 1. Constant folding
    int folded = 100 * 2 + 50;

    // 2. Algebraic simplification
    int zero_val = a * 0;

    // 3. Dead code (unused variable and calculation)
    int dead_math = b * 999 + 123;

    // 4. Common subexpressions
    int expr1 = a * b;
    int expr2 = a * b;

    // 5. Constant condition branch
    int result = 0;
    if (1) {
        result = folded + expr1 + expr2;
    } else {
        result = 99999;
    }

    return result;
}
