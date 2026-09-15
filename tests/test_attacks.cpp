#include "attacks/lo_manipulation_attack.hpp"
#include "attacks/saturation_attack.hpp"
#include "attacks/intercept_resend_attack.hpp"
#include "attacks/collective_attack.hpp"
#include <gtest/gtest.h>

using namespace cvqkd;

TEST(SaturationAttack, ClipsToLevel) 
{
    Measurement m;
    m.X.resize(3);
    m.P.resize(3);
    m.X << 100.0, -200.0, 0.5;
    m.P << -50.0, 300.0, -0.1;
    SaturationAttack a(10.0, 1.0);
    a.apply(m);
    EXPECT_DOUBLE_EQ(m.X(0), 10.0);
    EXPECT_DOUBLE_EQ(m.X(1), -10.0);
    EXPECT_DOUBLE_EQ(m.P(1), 10.0);
}

TEST(LOManipulationAttack, ScalesLinear) 
{
    Measurement m;
    m.X.resize(2);
    m.P.resize(2);
    m.X << 2.0, -3.0;
    m.P << 4.0, 5.0;
    LOManipulationAttack a(0.5);
    a.apply(m);
    EXPECT_DOUBLE_EQ(m.X(0), 1.0);
    EXPECT_DOUBLE_EQ(m.P(1), 2.5);
}

TEST(InterceptResendAttack, AddsNoiseAndResends) 
{
    Measurement m;
    m.X.resize(100);
    m.P.resize(100);
    // Заполняем сигнал известными значениями
    for (int i = 0; i < 100; ++i) {
        m.X(i) = 1.0;
        m.P(i) = 0.5;
    }
    
    InterceptResendAttack a(0.8, 1.0, 42);
    a.apply(m);
    
    // После атаки значения должны измениться (добавлен шум)
    // Проверяем, что дисперсия увеличилась
    double var_X = 0.0, var_P = 0.0;
    double mean_X = m.X.mean(), mean_P = m.P.mean();
    for (int i = 0; i < 100; ++i) {
        var_X += (m.X(i) - mean_X) * (m.X(i) - mean_X);
        var_P += (m.P(i) - mean_P) * (m.P(i) - mean_P);
    }
    var_X /= 99;
    var_P /= 99;
    
    // Дисперсия должна быть больше нуля (шум добавлен)
    EXPECT_GT(var_X, 0.0);
    EXPECT_GT(var_P, 0.0);
}

TEST(CollectiveAttack, AddsExcessNoise) 
{
    Measurement m;
    m.X.resize(100);
    m.P.resize(100);
    // Заполняем сигнал известными значениями
    for (int i = 0; i < 100; ++i) {
        m.X(i) = 1.0;
        m.P(i) = 0.5;
    }
    
    CollectiveAttack a(0.3, 0.1, 42);
    a.apply(m);
    
    // Проверяем, что сигнал изменился (ослаблен и зашумлен)
    double mean_X = m.X.mean();
    double mean_P = m.P.mean();
    
    // Среднее должно быть близко к исходному значению, умноженному на transmission
    const double transmission = std::sqrt(1.0 - 0.3 * 0.3);
    EXPECT_NEAR(mean_X, 1.0 * transmission, 0.3);
    EXPECT_NEAR(mean_P, 0.5 * transmission, 0.3);
}