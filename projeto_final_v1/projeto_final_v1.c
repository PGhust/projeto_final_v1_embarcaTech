#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/adc.h"
#include "ws2818b.pio.h"  // Biblioteca gerada pelo arquivo .pio durante compilação.

// Definição do número de LEDs e pino.
#define LED_COUNT 25
#define LED_PIN 7

// Definição de pixel GRB
struct pixel_t {
    uint8_t G, R, B; // Três valores de 8-bits compõem um pixel.
};
typedef struct pixel_t pixel_t;
typedef pixel_t npLED_t; // Mudança de nome de "struct pixel_t" para "npLED_t" por clareza.

// Declaração do buffer de pixels que formam a matriz.
npLED_t leds[LED_COUNT];

// Variáveis para uso da máquina PIO.
PIO np_pio;
uint sm;

// Inicializa a máquina PIO para controle da matriz de LEDs.
void npInit(uint pin) {
    uint offset = pio_add_program(pio0, &ws2818b_program);
    np_pio = pio0;

    sm = pio_claim_unused_sm(np_pio, false);
    if (sm < 0) {
        np_pio = pio1;
        sm = pio_claim_unused_sm(np_pio, true); // Se nenhuma máquina estiver livre, panic!
    }

    ws2818b_program_init(np_pio, sm, offset, pin, 800000.f);

    // Limpa buffer de pixels.
    for (uint i = 0; i < LED_COUNT; ++i) {
        leds[i].R = 0;
        leds[i].G = 0;
        leds[i].B = 0;
    }
}

// Atribui uma cor RGB a um LED.
void npSetLED(const uint index, const uint8_t r, const uint8_t g, const uint8_t b) {
    leds[index].R = r;
    leds[index].G = g;
    leds[index].B = b;
}

// Limpa o buffer de pixels.
void npClear() {
    for (uint i = 0; i < LED_COUNT; ++i)
        npSetLED(i, 0, 0, 0);
}

// Escreve os dados do buffer nos LEDs.
void npWrite() {
    for (uint i = 0; i < LED_COUNT; ++i) {
        pio_sm_put_blocking(np_pio, sm, leds[i].G);
        pio_sm_put_blocking(np_pio, sm, leds[i].R);
        pio_sm_put_blocking(np_pio, sm, leds[i].B);
    }
}

// Função para acender a cruz em azul
void drawCross() {
    int cross_indices[] = {2, 7, 10, 11, 12, 13, 14, 17, 22};
    for (int i = 0; i < sizeof(cross_indices) / sizeof(cross_indices[0]); i++) {
        npSetLED(cross_indices[i], 0, 0, 255); // Azul
    }
}

// Função para acender a losango em azul -> Futuramente colocar botões para alternância entre os padrões apresentados
// void drawLoss() {
//     int loss_indices[] = {2, 6, 8, 10, 14, 16, 22, 18};
//     for (int i = 0; i < sizeof(loss_indices) / sizeof(loss_indices[0]); i++) {
//         npSetLED(loss_indices[i], 0, 0, 255); // Azul
//     }
// }



int main() {
    // Inicializa entradas e saídas.
    stdio_init_all();

    // Inicializa matriz de LEDs NeoPixel.
    npInit(LED_PIN);
    npClear();

    // Inicializando o conversor Analógico-Digital (ADC)
    adc_init();
    adc_gpio_init(26); // Eixo Y
    adc_gpio_init(27); // Eixo X

    // Inicializa o LED na posição atual do joystick
    uint current_led_index = 0; // Inicializa o índice do LED atual
    npWrite(); // Escreve o estado inicial dos LEDs

    // Array para manter o controle dos LEDs vermelhos
    bool red_leds[LED_COUNT] = {false};

        // Desenha a cruz em azul
        drawCross();

        // Optional: Adiciona um pequeno atraso para observar o estado inicial do LED
        sleep_ms(1000); // Espera 1 segundo antes de começar o loop
    
        // Índices da cruz
        int cross_indices[] = {2, 7, 10, 11, 12, 13, 14, 17, 22};
        const int cross_size = sizeof(cross_indices) / sizeof(cross_indices[0]);
    
        while (true) {
            // Lê o valor do ADC para o eixo X
            adc_select_input(1);
            uint adc_x_raw = adc_read();
    
            // Lê valor do ADC para o eixo Y
            adc_select_input(0);
            uint adc_y_raw = adc_read();
    
            // Mapeia os valores do joystick para os índices dos LEDs
            const uint bar_width = 5; // Largura da matriz (5 LEDs)
            const uint bar_height = 5; // Altura da matriz (5 LEDs)
            const uint adc_max = (1 << 12) - 1; // Valor máximo do ADC (12 bits, então 4095)
    
            // Calcula a nova posição do LED a partir dos valores do joystick
            uint led_x = adc_x_raw * bar_width / adc_max; // Mapeia X
            uint led_y = adc_y_raw * bar_height / adc_max; // Mapeia Y
    
            // Calcula o índice do LED
            uint led_index = led_y * bar_width + led_x;
    
            // Limpa a matriz
            npClear();
    
            // Desenha a cruz em azul novamente
            drawCross();
    
            // Se o LED atual não estiver vermelho, torne-o vermelho
            if (!red_leds[current_led_index]) {
                npSetLED(current_led_index, 255, 0, 0); // Define a posição anterior como vermelha
                red_leds[current_led_index] = true; // Marca este LED como vermelho
            }
    
            // Verifica se o LED atual (cursor) está fora da cruz
            bool is_in_cross = false;
            for (int i = 0; i < cross_size; i++) {
                if (led_index == cross_indices[i]) {
                    is_in_cross = true;
                    break;
                }
            }
    
            // Se o LED atual estiver fora da cruz, torne-o vermelho
            if (!is_in_cross) {
                npSetLED(led_index, 255, 0, 0); // Define o LED correspondente para vermelho
            } else {
                npSetLED(led_index, 0, 255, 0); // Caso contrário, define como verde
            }
    
            // Atualiza o índice do LED atual
            current_led_index = led_index;
    
            // Escreve os estados atualizados dos LEDs
            npWrite(); // Escreve os dados nos LEDs
    
            // Pausa o programa por um curto período antes de ler novamente
            sleep_ms(100);
        }
    }