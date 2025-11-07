/*
    Otto机器人控制器 - MCP协议版本
*/

#include <cJSON.h>
#include <esp_log.h>

#include <cstdlib> 
#include <cstring>

#include "application.h"
#include "board.h"
#include "config.h"
#include "mcp_server.h"
#include "otto_movements.h"
#include "sdkconfig.h"
#include "settings.h"

#define TAG "OttoController"

class OttoController {
private:
    Otto otto_;
    TaskHandle_t action_task_handle_ = nullptr;
    QueueHandle_t action_queue_;
    bool has_hands_ = false;
    bool is_action_in_progress_ = false;

    struct OttoActionParams {
        int action_type;
        int steps;
        int speed;
        int direction;
        int amount;
        char servo_sequence_json[512];  // 用于存储舵机序列的JSON字符串
    };

    enum ActionType {
        ACTION_WALK = 1,
        ACTION_TURN = 2,
        ACTION_JUMP = 3,
        ACTION_SWING = 4,
        ACTION_MOONWALK = 5,
        ACTION_BEND = 6,
        ACTION_SHAKE_LEG = 7,
        ACTION_SIT = 25,  // 坐下
        ACTION_RADIO_CALISTHENICS = 26,  // 广播体操
        ACTION_MAGIC_CIRCLE = 27,  // 爱的魔力转圈圈
        ACTION_UPDOWN = 8,
        ACTION_TIPTOE_SWING = 9,
        ACTION_JITTER = 10,
        ACTION_ASCENDING_TURN = 11,
        ACTION_CRUSAITO = 12,
        ACTION_FLAPPING = 13,
        ACTION_HANDS_UP = 14,
        ACTION_HANDS_DOWN = 15,
        ACTION_HAND_WAVE = 16,
        ACTION_WINDMILL = 20,  // 大风车
        ACTION_TAKEOFF = 21,   // 起飞
        ACTION_FITNESS = 22,   // 健身
        ACTION_GREETING = 23,  // 打招呼
        ACTION_SHY = 24,        // 害羞
        ACTION_SHOWCASE = 28,   // 展示动作
        ACTION_HOME = 17,
        ACTION_SERVO_SEQUENCE = 18,  // 舵机序列（自编程）
        ACTION_WHIRLWIND_LEG = 19    // 旋风腿
    };

    static void ActionTask(void* arg) {
        OttoController* controller = static_cast<OttoController*>(arg);
        OttoActionParams params;
        controller->otto_.AttachServos();

        while (true) {
            if (xQueueReceive(controller->action_queue_, &params, pdMS_TO_TICKS(1000)) == pdTRUE) {
                ESP_LOGI(TAG, "Execute action: %d", params.action_type);
                controller->is_action_in_progress_ = true;
                if (params.action_type == ACTION_SERVO_SEQUENCE) {
                    // 执行舵机序列（自编程）- 仅支持短键名格式
                    cJSON* json = cJSON_Parse(params.servo_sequence_json);
                    if (json != nullptr) {
                        ESP_LOGD(TAG, "JSON解析成功，长度=%d", strlen(params.servo_sequence_json));
                        // 使用短键名 "a" 表示动作数组
                        cJSON* actions = cJSON_GetObjectItem(json, "a");
                        if (cJSON_IsArray(actions)) {
                            int array_size = cJSON_GetArraySize(actions);
                            ESP_LOGI(TAG, "执行舵机序列，共%d个动作", array_size);
                            
                            // 获取序列执行完成后的延迟（短键名 "d"，顶层参数）
                            int sequence_delay = 0;
                            cJSON* delay_item = cJSON_GetObjectItem(json, "d");
                            if (cJSON_IsNumber(delay_item)) {
                                sequence_delay = delay_item->valueint;
                                if (sequence_delay < 0) sequence_delay = 0;
                            }
                            
                            // 初始化当前舵机位置（用于保持未指定的舵机位置）
                            int current_positions[SERVO_COUNT];
                            for (int j = 0; j < SERVO_COUNT; j++) {
                                current_positions[j] = 90;  // 默认中间位置
                            }
                            // 手部舵机默认位置
                            current_positions[LEFT_HAND] = 45;
                            current_positions[RIGHT_HAND] = 180 - 45;
                            
                            for (int i = 0; i < array_size; i++) {
                                cJSON* action_item = cJSON_GetArrayItem(actions, i);
                                if (cJSON_IsObject(action_item)) {
                                    // 检查是否为振荡器模式（短键名 "osc"）
                                    cJSON* osc_item = cJSON_GetObjectItem(action_item, "osc");
                                    if (cJSON_IsObject(osc_item)) {
                                        // 振荡器模式 - 使用Execute2，以绝对角度为中心振荡
                                        int amplitude[SERVO_COUNT] = {0};
                                        int center_angle[SERVO_COUNT] = {0};
                                        double phase_diff[SERVO_COUNT] = {0};
                                        int period = 500;  // 默认周期500毫秒
                                        float steps = 5.0;  // 默认步数5.0
                                        
                                        const char* servo_names[] = {"ll", "rl", "lf", "rf", "lh", "rh"};
                                        
                                        // 读取振幅（短键名 "a"），默认20度
                                        for (int j = 0; j < SERVO_COUNT; j++) {
                                            amplitude[j] = 20;  // 默认振幅20度
                                        }
                                        cJSON* amp_item = cJSON_GetObjectItem(osc_item, "a");
                                        if (cJSON_IsObject(amp_item)) {
                                            for (int j = 0; j < SERVO_COUNT; j++) {
                                                cJSON* amp_value = cJSON_GetObjectItem(amp_item, servo_names[j]);
                                                if (cJSON_IsNumber(amp_value)) {
                                                    int amp = amp_value->valueint;
                                                    if (amp >= 10 && amp <= 90) {
                                                        amplitude[j] = amp;
                                                    }
                                                }
                                            }
                                        }
                                        
                                        // 读取中心角度（短键名 "o"），默认90度（绝对角度0-180度）
                                        for (int j = 0; j < SERVO_COUNT; j++) {
                                            center_angle[j] = 90;  // 默认中心角度90度（中间位置）
                                        }
                                        cJSON* center_item = cJSON_GetObjectItem(osc_item, "o");
                                        if (cJSON_IsObject(center_item)) {
                                            for (int j = 0; j < SERVO_COUNT; j++) {
                                                cJSON* center_value = cJSON_GetObjectItem(center_item, servo_names[j]);
                                                if (cJSON_IsNumber(center_value)) {
                                                    int center = center_value->valueint;
                                                    if (center >= 0 && center <= 180) {
                                                        center_angle[j] = center;
                                                    }
                                                }
                                            }
                                        }
                                        
                                        // 安全检查：防止左右腿脚同时做大幅度振荡（振幅检查）
                                        const int LARGE_AMPLITUDE_THRESHOLD = 40;  // 大幅度振幅阈值：40度
                                        bool left_leg_large = amplitude[LEFT_LEG] >= LARGE_AMPLITUDE_THRESHOLD;
                                        bool right_leg_large = amplitude[RIGHT_LEG] >= LARGE_AMPLITUDE_THRESHOLD;
                                        bool left_foot_large = amplitude[LEFT_FOOT] >= LARGE_AMPLITUDE_THRESHOLD;
                                        bool right_foot_large = amplitude[RIGHT_FOOT] >= LARGE_AMPLITUDE_THRESHOLD;
                                        
                                        if (left_leg_large && right_leg_large) {
                                            ESP_LOGW(TAG, "检测到左右腿同时大幅度振荡，限制右腿振幅");
                                            amplitude[RIGHT_LEG] = 0;  // 禁止右腿振荡
                                        }
                                        if (left_foot_large && right_foot_large) {
                                            ESP_LOGW(TAG, "检测到左右脚同时大幅度振荡，限制右脚振幅");
                                            amplitude[RIGHT_FOOT] = 0;  // 禁止右脚振荡
                                        }
                                        
                                        // 读取相位差（短键名 "ph"，单位为度，转换为弧度）
                                        cJSON* phase_item = cJSON_GetObjectItem(osc_item, "ph");
                                        if (cJSON_IsObject(phase_item)) {
                                            for (int j = 0; j < SERVO_COUNT; j++) {
                                                cJSON* phase_value = cJSON_GetObjectItem(phase_item, servo_names[j]);
                                                if (cJSON_IsNumber(phase_value)) {
                                                    // 将度数转换为弧度
                                                    phase_diff[j] = phase_value->valuedouble * 3.141592653589793 / 180.0;
                                                }
                                            }
                                        }
                                        
                                        // 读取周期（短键名 "p"），范围100-3000毫秒
                                        cJSON* period_item = cJSON_GetObjectItem(osc_item, "p");
                                        if (cJSON_IsNumber(period_item)) {
                                            period = period_item->valueint;
                                            if (period < 100) period = 100;
                                            if (period > 3000) period = 3000;  // 与描述一致，限制3000毫秒
                                        }
                                        
                                        // 读取周期数（短键名 "c"），范围0.1-20.0
                                        cJSON* steps_item = cJSON_GetObjectItem(osc_item, "c");
                                        if (cJSON_IsNumber(steps_item)) {
                                            steps = (float)steps_item->valuedouble;
                                            if (steps < 0.1) steps = 0.1;
                                            if (steps > 20.0) steps = 20.0;  // 与描述一致，限制20.0
                                        }
                                        
                                        // 执行振荡 - 使用Execute2，以绝对角度为中心
                                        ESP_LOGI(TAG, "执行振荡动作%d: period=%d, steps=%.1f", i, period, steps);
                                        controller->otto_.Execute2(amplitude, center_angle, period, phase_diff, steps);
                                        
                                        // 振荡后更新位置（使用center_angle作为最终位置）
                                        for (int j = 0; j < SERVO_COUNT; j++) {
                                            current_positions[j] = center_angle[j];
                                        }
                                    } else {
                                        // 普通移动模式
                                        // 从当前位置数组复制，保持未指定的舵机位置
                                        int servo_target[SERVO_COUNT];
                                        for (int j = 0; j < SERVO_COUNT; j++) {
                                            servo_target[j] = current_positions[j];
                                        }
                                        
                                        // 从JSON中读取舵机位置（短键名 "s"）
                                        cJSON* servos_item = cJSON_GetObjectItem(action_item, "s");
                                        if (cJSON_IsObject(servos_item)) {
                                            // 短键名：ll/rl/lf/rf/lh/rh
                                            const char* servo_names[] = {"ll", "rl", "lf", "rf", "lh", "rh"};
                                            
                                            for (int j = 0; j < SERVO_COUNT; j++) {
                                                cJSON* servo_value = cJSON_GetObjectItem(servos_item, servo_names[j]);
                                                if (cJSON_IsNumber(servo_value)) {
                                                    int position = servo_value->valueint;
                                                    // 限制位置范围在0-180度
                                                    if (position >= 0 && position <= 180) {
                                                        servo_target[j] = position;
                                                    }
                                                }
                                            }
                                        }
                                        
                                        // 安全检查：防止左右腿脚同时做大幅度动作
                                        const int LARGE_MOVEMENT_THRESHOLD = 40;  // 大幅度动作阈值：40度
                                        bool left_leg_large = abs(servo_target[LEFT_LEG] - current_positions[LEFT_LEG]) >= LARGE_MOVEMENT_THRESHOLD;
                                        bool right_leg_large = abs(servo_target[RIGHT_LEG] - current_positions[RIGHT_LEG]) >= LARGE_MOVEMENT_THRESHOLD;
                                        bool left_foot_large = abs(servo_target[LEFT_FOOT] - current_positions[LEFT_FOOT]) >= LARGE_MOVEMENT_THRESHOLD;
                                        bool right_foot_large = abs(servo_target[RIGHT_FOOT] - current_positions[RIGHT_FOOT]) >= LARGE_MOVEMENT_THRESHOLD;
                                        
                                        if (left_leg_large && right_leg_large) {
                                            ESP_LOGW(TAG, "检测到左右腿同时大幅度动作，限制右腿动作");
                                            // 保持右腿在原位置
                                            servo_target[RIGHT_LEG] = current_positions[RIGHT_LEG];
                                        }
                                        if (left_foot_large && right_foot_large) {
                                            ESP_LOGW(TAG, "检测到左右脚同时大幅度动作，限制右脚动作");
                                            // 保持右脚在原位置
                                            servo_target[RIGHT_FOOT] = current_positions[RIGHT_FOOT];
                                        }
                                        
                                        // 获取移动速度（短键名 "v"，默认1000毫秒）
                                        int speed = 1000;
                                        cJSON* speed_item = cJSON_GetObjectItem(action_item, "v");
                                        if (cJSON_IsNumber(speed_item)) {
                                            speed = speed_item->valueint;
                                            if (speed < 100) speed = 100;  // 最小100毫秒
                                            if (speed > 3000) speed = 3000;  // 最大3000毫秒
                                        }
                                        
                                        // 执行舵机移动
                                        ESP_LOGI(TAG, "执行动作%d: ll=%d, rl=%d, lf=%d, rf=%d, v=%d",
                                                 i, servo_target[LEFT_LEG], servo_target[RIGHT_LEG],
                                                 servo_target[LEFT_FOOT], servo_target[RIGHT_FOOT], speed);
                                        controller->otto_.MoveServos(speed, servo_target);
                                        
                                        // 更新当前位置数组，用于下一个动作
                                        for (int j = 0; j < SERVO_COUNT; j++) {
                                            current_positions[j] = servo_target[j];
                                        }
                                    }
                                    
                                    // 获取动作后的延迟时间（短键名 "d"）
                                    int delay_after = 0;
                                    cJSON* delay_item = cJSON_GetObjectItem(action_item, "d");
                                    if (cJSON_IsNumber(delay_item)) {
                                        delay_after = delay_item->valueint;
                                        if (delay_after < 0) delay_after = 0;
                                    }
                                    
                                    // 动作后的延迟（最后一个动作后不延迟）
                                    if (delay_after > 0 && i < array_size - 1) {
                                        ESP_LOGI(TAG, "动作%d执行完成，延迟%d毫秒", i, delay_after);
                                        vTaskDelay(pdMS_TO_TICKS(delay_after));
                                    }
                                }
                            }
                            
                            // 序列执行完成后的延迟（用于序列之间的停顿）
                            if (sequence_delay > 0) {
                                // 检查队列中是否还有待执行的序列
                                UBaseType_t queue_count = uxQueueMessagesWaiting(controller->action_queue_);
                                if (queue_count > 0) {
                                    ESP_LOGI(TAG, "序列执行完成，延迟%d毫秒后执行下一个序列（队列中还有%d个序列）", 
                                             sequence_delay, queue_count);
                                    vTaskDelay(pdMS_TO_TICKS(sequence_delay));
                                }
                            }
                            // 释放JSON内存
                            cJSON_Delete(json);
                        } else {
                            ESP_LOGE(TAG, "舵机序列格式错误: 'a'不是数组");
                            cJSON_Delete(json);
                        }
                    } else {
                        // 获取cJSON的错误信息
                        const char* error_ptr = cJSON_GetErrorPtr();
                        int json_len = strlen(params.servo_sequence_json);
                        ESP_LOGE(TAG, "解析舵机序列JSON失败，长度=%d，错误位置: %s", json_len, 
                                 error_ptr ? error_ptr : "未知");
                        ESP_LOGE(TAG, "JSON内容: %s", params.servo_sequence_json);
                    }
                } else {
                    // 执行预定义动作
                    switch (params.action_type) {
                        case ACTION_WALK:
                            controller->otto_.Walk(params.steps, params.speed, params.direction,
                                                   params.amount);
                            break;
                        case ACTION_TURN:
                            controller->otto_.Turn(params.steps, params.speed, params.direction,
                                                   params.amount);
                            break;
                        case ACTION_JUMP:
                            controller->otto_.Jump(params.steps, params.speed);
                            break;
                        case ACTION_SWING:
                            controller->otto_.Swing(params.steps, params.speed, params.amount);
                            break;
                        case ACTION_MOONWALK:
                            controller->otto_.Moonwalker(params.steps, params.speed, params.amount,
                                                         params.direction);
                            break;
                        case ACTION_BEND:
                            controller->otto_.Bend(params.steps, params.speed, params.direction);
                            break;
                        case ACTION_SHAKE_LEG:
                            controller->otto_.ShakeLeg(params.steps, params.speed, params.direction);
                            break;
                        case ACTION_SIT:
                            controller->otto_.Sit();
                            break;
                        case ACTION_RADIO_CALISTHENICS:
                            if (controller->has_hands_) {
                                controller->otto_.RadioCalisthenics();
                            }
                            break;
                        case ACTION_MAGIC_CIRCLE:
                            if (controller->has_hands_) {
                                controller->otto_.MagicCircle();
                            }
                            break;
                        case ACTION_SHOWCASE:
                            controller->otto_.Showcase();
                            break;
                        case ACTION_UPDOWN:
                            controller->otto_.UpDown(params.steps, params.speed, params.amount);
                            break;
                        case ACTION_TIPTOE_SWING:
                            controller->otto_.TiptoeSwing(params.steps, params.speed, params.amount);
                            break;
                        case ACTION_JITTER:
                            controller->otto_.Jitter(params.steps, params.speed, params.amount);
                            break;
                        case ACTION_ASCENDING_TURN:
                            controller->otto_.AscendingTurn(params.steps, params.speed, params.amount);
                            break;
                        case ACTION_CRUSAITO:
                            controller->otto_.Crusaito(params.steps, params.speed, params.amount,
                                                       params.direction);
                            break;
                        case ACTION_FLAPPING:
                            controller->otto_.Flapping(params.steps, params.speed, params.amount,
                                                       params.direction);
                            break;
                        case ACTION_WHIRLWIND_LEG:
                            controller->otto_.WhirlwindLeg(params.steps, params.speed, params.amount);
                            break;
                        case ACTION_HANDS_UP:
                            if (controller->has_hands_) {
                                controller->otto_.HandsUp(params.speed, params.direction);
                            }
                            break;
                        case ACTION_HANDS_DOWN:
                            if (controller->has_hands_) {
                                controller->otto_.HandsDown(params.speed, params.direction);
                            }
                            break;
                        case ACTION_HAND_WAVE:
                            if (controller->has_hands_) {
                                controller->otto_.HandWave( params.direction);
                            }
                            break;
                        case ACTION_WINDMILL:
                            if (controller->has_hands_) {
                                controller->otto_.Windmill(params.steps, params.speed, params.amount);
                            }
                            break;
                        case ACTION_TAKEOFF:
                            if (controller->has_hands_) {
                                controller->otto_.Takeoff(params.steps, params.speed, params.amount);
                            }
                            break;
                        case ACTION_FITNESS:
                            if (controller->has_hands_) {
                                controller->otto_.Fitness(params.steps, params.speed, params.amount);
                            }
                            break;
                        case ACTION_GREETING:
                            if (controller->has_hands_) {
                                controller->otto_.Greeting(params.direction, params.steps);
                            }
                            break;
                        case ACTION_SHY:
                            if (controller->has_hands_) {
                                controller->otto_.Shy(params.direction, params.steps);
                            }
                            break;
                        case ACTION_HOME:
                            controller->otto_.Home(true);
                            break;
                    }
                    if(params.action_type != ACTION_SIT){
                        if (params.action_type != ACTION_HOME && params.action_type != ACTION_SERVO_SEQUENCE) {
                            controller->otto_.Home(params.action_type != ACTION_HANDS_UP);
                        }
                    }
                }
                controller->is_action_in_progress_ = false;
                vTaskDelay(pdMS_TO_TICKS(20));
            }
        }
    }

    void StartActionTaskIfNeeded() {
        if (action_task_handle_ == nullptr) {
            xTaskCreate(ActionTask, "otto_action", 1024 * 3, this, configMAX_PRIORITIES - 1,
                        &action_task_handle_);
        }
    }

    void QueueAction(int action_type, int steps, int speed, int direction, int amount) {
        // 检查手部动作
        if ((action_type >= ACTION_HANDS_UP && action_type <= ACTION_HAND_WAVE) || 
            (action_type == ACTION_WINDMILL) || (action_type == ACTION_TAKEOFF) || 
            (action_type == ACTION_FITNESS) || (action_type == ACTION_GREETING) ||
            (action_type == ACTION_SHY) || (action_type == ACTION_RADIO_CALISTHENICS) ||
            (action_type == ACTION_MAGIC_CIRCLE)) {
            if (!has_hands_) {
                ESP_LOGW(TAG, "尝试执行手部动作，但机器人没有配置手部舵机");
                return;
            }
        }

        ESP_LOGI(TAG, "Action control: type=%d, steps=%d, speed=%d, direction=%d, amount=%d", action_type, steps,
                 speed, direction, amount);

        OttoActionParams params = {action_type, steps, speed, direction, amount, ""};
        xQueueSend(action_queue_, &params, portMAX_DELAY);
        StartActionTaskIfNeeded();
    }

    void QueueServoSequence(const char* servo_sequence_json) {
        if (servo_sequence_json == nullptr) {
            ESP_LOGE(TAG, "序列JSON为空");
            return;
        }
        
        int input_len = strlen(servo_sequence_json);
        const int buffer_size = 512;  // servo_sequence_json数组大小
        ESP_LOGI(TAG, "队列舵机序列，输入长度=%d，缓冲区大小=%d", input_len, buffer_size);
        
        if (input_len >= buffer_size) {
            ESP_LOGE(TAG, "JSON字符串太长！输入长度=%d，最大允许=%d", input_len, buffer_size - 1);
            return;
        }
        
        if (input_len == 0) {
            ESP_LOGW(TAG, "序列JSON为空字符串");
            return;
        }
        
        OttoActionParams params = {ACTION_SERVO_SEQUENCE, 0, 0, 0, 0, ""};
        // 复制JSON字符串到结构体中（限制长度）
        strncpy(params.servo_sequence_json, servo_sequence_json, sizeof(params.servo_sequence_json) - 1);
        params.servo_sequence_json[sizeof(params.servo_sequence_json) - 1] = '\0';
        
        ESP_LOGD(TAG, "序列已加入队列: %s", params.servo_sequence_json);
        
        xQueueSend(action_queue_, &params, portMAX_DELAY);
        StartActionTaskIfNeeded();
    }

    void LoadTrimsFromNVS() {
        Settings settings("otto_trims", false);

        int left_leg = settings.GetInt("left_leg", 0);
        int right_leg = settings.GetInt("right_leg", 0);
        int left_foot = settings.GetInt("left_foot", 0);
        int right_foot = settings.GetInt("right_foot", 0);
        int left_hand = settings.GetInt("left_hand", 0);
        int right_hand = settings.GetInt("right_hand", 0);

        ESP_LOGI(TAG, "Load fine-tuning settings from NVS: left leg=%d, right leg=%d, left foot=%d, right foot=%d, left hand=%d, right hand=%d",
                 left_leg, right_leg, left_foot, right_foot, left_hand, right_hand);

        otto_.SetTrims(left_leg, right_leg, left_foot, right_foot, left_hand, right_hand);
    }

public:
    OttoController() {
        otto_.Init(LEFT_LEG_PIN, RIGHT_LEG_PIN, LEFT_FOOT_PIN, RIGHT_FOOT_PIN, LEFT_HAND_PIN,
                   RIGHT_HAND_PIN);

        has_hands_ = (LEFT_HAND_PIN != -1 && RIGHT_HAND_PIN != -1);
        ESP_LOGI(TAG, "Otto robot initialization %s hand servos", has_hands_ ? "with" : "without");

        LoadTrimsFromNVS();

        action_queue_ = xQueueCreate(10, sizeof(OttoActionParams));

        QueueAction(ACTION_HOME, 1, 1000, 1, 0);  // direction=1表示复位手部

        RegisterMcpTools();
    }

    void RegisterMcpTools() {
        auto& mcp_server = McpServer::GetInstance();

        ESP_LOGI(TAG, "Start registering MCP tools...");

        // Basic movement actions
        mcp_server.AddTool("self.otto.walk_forward",
                           "Walking. Steps: Steps taken(1-100); speed: Walking speed(500-1500, lower is faster); "
                           "direction: Walking direction(-1=backward, 1=forward); arm_swing: Arm swing angle(0-170 degrees)",
                           PropertyList({Property("steps", kPropertyTypeInteger, 3, 1, 100),
                                         Property("speed", kPropertyTypeInteger, 1000, 500, 1500),
                                         Property("arm_swing", kPropertyTypeInteger, 50, 0, 170),
                                         Property("direction", kPropertyTypeInteger, 1, -1, 1)}),
                           [this](const PropertyList& properties) -> ReturnValue {
                               int steps = properties["steps"].value<int>();
                               int speed = properties["speed"].value<int>();
                               int arm_swing = properties["arm_swing"].value<int>();
                               int direction = properties["direction"].value<int>();
                               QueueAction(ACTION_WALK, steps, speed, direction, arm_swing);
                               return true;
                           });

        mcp_server.AddTool("self.otto.turn_left",
                           "Turning. Steps: Steps taken(1-100); speed: Turning speed(500-1500, lower is faster); "
                           "direction: Turning direction(1=left, -1=right); arm_swing: Arm swing angle(0-170 degrees)",
                           PropertyList({Property("steps", kPropertyTypeInteger, 3, 1, 100),
                                         Property("speed", kPropertyTypeInteger, 1000, 500, 1500),
                                         Property("arm_swing", kPropertyTypeInteger, 50, 0, 170),
                                         Property("direction", kPropertyTypeInteger, 1, -1, 1)}),
                           [this](const PropertyList& properties) -> ReturnValue {
                               int steps = properties["steps"].value<int>();
                               int speed = properties["speed"].value<int>();
                               int arm_swing = properties["arm_swing"].value<int>();
                               int direction = properties["direction"].value<int>();
                               QueueAction(ACTION_TURN, steps, speed, direction, arm_swing);
                               return true;
                           });

        mcp_server.AddTool("self.otto.jump",
                           "Jumping. Steps: Steps taken(1-100); speed: Jumping speed(500-1500, lower is faster)",
                           PropertyList({Property("steps", kPropertyTypeInteger, 1, 1, 100),
                                         Property("speed", kPropertyTypeInteger, 1000, 500, 1500)}),
                           [this](const PropertyList& properties) -> ReturnValue {
                               int steps = properties["steps"].value<int>();
                               int speed = properties["speed"].value<int>();
                               QueueAction(ACTION_JUMP, steps, speed, 0, 0);
                               return true;
                           });

        // Special Actions
        mcp_server.AddTool("self.otto.swing",
                           "Swinging. Steps: Steps taken(1-100); speed: "
                           "Swinging speed(500-1500, lower is faster); amount: Swinging angle(0-170 degrees)",
                           PropertyList({Property("steps", kPropertyTypeInteger, 3, 1, 100),
                                         Property("speed", kPropertyTypeInteger, 1000, 500, 1500),
                                         Property("amount", kPropertyTypeInteger, 30, 0, 170)}),
                           [this](const PropertyList& properties) -> ReturnValue {
                               int steps = properties["steps"].value<int>();
                               int speed = properties["speed"].value<int>();
                               int amount = properties["amount"].value<int>();
                               QueueAction(ACTION_SWING, steps, speed, 0, amount);
                               return true;
                           });

        mcp_server.AddTool("self.otto.moonwalk",
                           "Moonwalking. Steps: Steps taken(1-100); speed: Speed(500-1500, lower is faster); "
                           "direction: Direction(1=left, -1=right); amount: Amplitude(0-170 degrees)",
                           PropertyList({Property("steps", kPropertyTypeInteger, 3, 1, 100),
                                         Property("speed", kPropertyTypeInteger, 1000, 500, 1500),
                                         Property("direction", kPropertyTypeInteger, 1, -1, 1),
                                         Property("amount", kPropertyTypeInteger, 25, 0, 170)}),
                           [this](const PropertyList& properties) -> ReturnValue {
                               int steps = properties["steps"].value<int>();
                               int speed = properties["speed"].value<int>();
                               int direction = properties["direction"].value<int>();
                               int amount = properties["amount"].value<int>();
                               QueueAction(ACTION_MOONWALK, steps, speed, direction, amount);
                               return true;
                           });

        mcp_server.AddTool("self.otto.bend",
                           "Bending. Steps: Steps taken(1-100); speed: Bending speed(500-1500, lower is faster); direction: Bending direction(1=left, -1=right)",
                           PropertyList({Property("steps", kPropertyTypeInteger, 1, 1, 100),
                                         Property("speed", kPropertyTypeInteger, 1000, 500, 1500),
                                         Property("direction", kPropertyTypeInteger, 1, -1, 1)}),
                           [this](const PropertyList& properties) -> ReturnValue {
                               int steps = properties["steps"].value<int>();
                               int speed = properties["speed"].value<int>();
                               int direction = properties["direction"].value<int>();
                               QueueAction(ACTION_BEND, steps, speed, direction, 0);
                               return true;
                           });

        mcp_server.AddTool("self.otto.shake_leg",
                           "Shaking legs. Steps: Steps taken(1-100); speed: Shaking speed(500-1500, lower is faster); "
                           "direction: Leg selection(1=left leg, -1=right leg)",
                           PropertyList({Property("steps", kPropertyTypeInteger, 1, 1, 100),
                                         Property("speed", kPropertyTypeInteger, 1000, 500, 1500),
                                         Property("direction", kPropertyTypeInteger, 1, -1, 1)}),
                           [this](const PropertyList& properties) -> ReturnValue {
                               int steps = properties["steps"].value<int>();
                               int speed = properties["speed"].value<int>();
                               int direction = properties["direction"].value<int>();
                               QueueAction(ACTION_SHAKE_LEG, steps, speed, direction, 0);
                               return true;
                           });

        mcp_server.AddTool("self.otto.sit",
                           "Sit down. No parameters needed.",
                           PropertyList(),
                           [this](const PropertyList &properties) -> ReturnValue
                           {
                               QueueAction(ACTION_SIT, 1, 0, 0, 0);
                               return true;
                           });

        mcp_server.AddTool("self.otto.showcase",
                           "Demonstrate actions. Perform multiple actions in sequence: walk forward 3 steps, wave, dance (calisthenics), moonwalk, sway, take off, exercise, walk backward 3 steps. No parameters required.",
                           PropertyList(),
                           [this](const PropertyList &properties) -> ReturnValue
                           {
                               QueueAction(ACTION_SHOWCASE, 1, 0, 0, 0);
                               return true;
                           });

        mcp_server.AddTool("self.otto.updown",
                           "Up and down movement. Steps: Steps taken(1-100); speed: "
                           "Movement speed(500-1500, lower is faster); amount: Movement amplitude(0-170 degrees)",
                           PropertyList({Property("steps", kPropertyTypeInteger, 3, 1, 100),
                                         Property("speed", kPropertyTypeInteger, 1000, 500, 1500),
                                         Property("amount", kPropertyTypeInteger, 20, 0, 170)}),
                           [this](const PropertyList& properties) -> ReturnValue {
                               int steps = properties["steps"].value<int>();
                               int speed = properties["speed"].value<int>();
                               int amount = properties["amount"].value<int>();
                               QueueAction(ACTION_UPDOWN, steps, speed, 0, amount);
                               return true;
                           });

        mcp_server.AddTool("self.otto.whirlwind_leg",
                           "Whirlwind leg."
                           "steps: Number of actions (3-100); speed: Action speed (100-1000, lower is faster, recommended 300); "
                           "amplitude: Kicking amplitude (20-40 degrees)",
                           PropertyList({Property("steps", kPropertyTypeInteger, 3, 3, 100),
                                         Property("speed", kPropertyTypeInteger, 300, 100, 1000),
                                         Property("amplitude", kPropertyTypeInteger, 30, 20, 40)}),
                           [this](const PropertyList& properties) -> ReturnValue {
                               int steps = properties["steps"].value<int>();
                               int speed = properties["speed"].value<int>();
                               int amplitude = properties["amplitude"].value<int>();
                               QueueAction(ACTION_WHIRLWIND_LEG, steps, speed, 0, amplitude);
                               return true;
                           });

        // 手部动作（仅在有手部舵机时可用）
        if (has_hands_) {
            mcp_server.AddTool(
                "self.otto.hands_up",
                "Hands up. Speed: Speed(500-1500, lower is faster); direction: Hand selection(1=left hand, "
                "-1=right hand, 0=both hands)",
                PropertyList({Property("speed", kPropertyTypeInteger, 1000, 500, 1500),
                              Property("direction", kPropertyTypeInteger, 1, -1, 1)}),
                [this](const PropertyList& properties) -> ReturnValue {
                    int speed = properties["speed"].value<int>();
                    int direction = properties["direction"].value<int>();
                    QueueAction(ACTION_HANDS_UP, 1, speed, direction, 0);
                    return true;
                });

            mcp_server.AddTool(
                "self.otto.hands_down",
                "Hands down. Speed: Speed(500-1500, lower is faster); direction: Hand selection(1=left hand, "
                "-1=right hand, 0=both hands)",
                PropertyList({Property("speed", kPropertyTypeInteger, 1000, 500, 1500),
                              Property("direction", kPropertyTypeInteger, 1, -1, 1)}),
                [this](const PropertyList& properties) -> ReturnValue {
                    int speed = properties["speed"].value<int>();
                    int direction = properties["direction"].value<int>();
                    QueueAction(ACTION_HANDS_DOWN, 1, speed, direction, 0);
                    return true;
                });

            mcp_server.AddTool(
                "self.otto.hand_wave",
                "Wave hand. direction: Hand selection (1=left hand, -1=right hand, 0=both hands)",
                PropertyList({Property("direction", kPropertyTypeInteger, 1, -1, 1)}),
                [this](const PropertyList& properties) -> ReturnValue {
                    int direction = properties["direction"].value<int>();   
                    QueueAction(ACTION_HAND_WAVE, 1, 0, 0, direction);
                    return true;
                });   

            mcp_server.AddTool(
                "self.otto.windmill",
                "Windmill. steps: Number of actions (3-100); "
                "speed: Action period (300-2000 ms, lower is faster); amplitude: Oscillation amplitude (50-90 degrees)",
                PropertyList({Property("steps", kPropertyTypeInteger, 6, 3, 100),
                              Property("speed", kPropertyTypeInteger, 500, 300, 2000),
                              Property("amplitude", kPropertyTypeInteger, 70, 50, 90)}),
                [this](const PropertyList& properties) -> ReturnValue {
                    int steps = properties["steps"].value<int>();
                    int speed = properties["speed"].value<int>();
                    int amplitude = properties["amplitude"].value<int>();
                    QueueAction(ACTION_WINDMILL, steps, speed, 0, amplitude);
                    return true;
                });

            mcp_server.AddTool(
                "self.otto.takeoff",
                "Take off. Both hands at 90 degrees position, oscillating in phase to simulate take-off action. steps: Number of actions (5-100); "
                "speed: Action period (200-600 ms, lower is faster, recommended 300); amplitude: Oscillation amplitude (20-60 degrees)",
                PropertyList({Property("steps", kPropertyTypeInteger, 5, 5, 100),
                              Property("speed", kPropertyTypeInteger, 300, 200, 600),
                              Property("amplitude", kPropertyTypeInteger, 40, 20, 60)}),
                [this](const PropertyList& properties) -> ReturnValue {
                    int steps = properties["steps"].value<int>();
                    int speed = properties["speed"].value<int>();
                    int amplitude = properties["amplitude"].value<int>();
                    QueueAction(ACTION_TAKEOFF, steps, speed, 0, amplitude);
                    return true;
                });

            mcp_server.AddTool(
                "self.otto.fitness",
                "Fitness. steps: Number of actions (3-100); speed: Action speed (500-2000 ms, lower is faster); amplitude: Oscillation amplitude (10-50 degrees)",
                PropertyList({Property("steps", kPropertyTypeInteger, 5, 3, 100),
                              Property("speed", kPropertyTypeInteger, 1000, 500, 2000),
                              Property("amplitude", kPropertyTypeInteger, 25, 10, 50)}),
                [this](const PropertyList& properties) -> ReturnValue {
                    int steps = properties["steps"].value<int>();
                    int speed = properties["speed"].value<int>();
                    int amplitude = properties["amplitude"].value<int>();
                    QueueAction(ACTION_FITNESS, steps, speed, 0, amplitude);
                    return true;
                });

            mcp_server.AddTool(
                "self.otto.greeting",
                "Wave hello. direction: Hand selection (1=left hand, -1=right hand); steps: Number of actions (3-100)",
                PropertyList({Property("direction", kPropertyTypeInteger, 1, -1, 1),
                              Property("steps", kPropertyTypeInteger, 5, 3, 100)}),
                [this](const PropertyList& properties) -> ReturnValue {
                    int direction = properties["direction"].value<int>();
                    int steps = properties["steps"].value<int>();
                    QueueAction(ACTION_GREETING, steps, 0, direction, 0);
                    return true;
                });

            mcp_server.AddTool(
                "self.otto.shy",
                "Shy. direction: Direction (1=left, -1=right); steps: Number of actions (3-100)",
                PropertyList({Property("direction", kPropertyTypeInteger, 1, -1, 1),
                              Property("steps", kPropertyTypeInteger, 5, 3, 100)}),
                [this](const PropertyList& properties) -> ReturnValue {
                    int direction = properties["direction"].value<int>();
                    int steps = properties["steps"].value<int>();
                    QueueAction(ACTION_SHY, steps, 0, direction, 0);
                    return true;
                });

            mcp_server.AddTool("self.otto.radio_calisthenics",
                               "Radio calisthenics. No parameters required.",
                               PropertyList(),
                               [this](const PropertyList& properties) -> ReturnValue {
                                   QueueAction(ACTION_RADIO_CALISTHENICS, 1, 0, 0, 0);
                                   return true;
                               });

            mcp_server.AddTool("self.otto.magic_circle",
                               "Magic circle of love. No parameters required.",
                               PropertyList(),
                               [this](const PropertyList& properties) -> ReturnValue {
                                   QueueAction(ACTION_MAGIC_CIRCLE, 1, 0, 0, 0);
                                   return true;
                               });
        }

        // 舵机序列工具（支持分段发送，每次发送一个序列，自动排队执行）
        mcp_server.AddTool(
            "self.otto.servo_sequences",
            "Control each servo to achieve autonomous action programming. Supports segmented sending sequences: AI can continuously call this tool multiple times, each time sending a short sequence, and the system will automatically queue and execute them in order. Supports both normal movement and oscillator modes."
            "Robot structure: Both hands can swing up and down, both legs can retract and extend, and both feet can flip up and down."
            "Servo description:"
            "ll(Left leg): Adduction and abduction, 0 degrees = fully abducted, 90 degrees = neutral, 180 degrees = fully adducted;"
            "rl(Right leg): Adduction and abduction, 0 degrees = fully adducted, 90 degrees = neutral, 180 degrees = fully abducted;"
            "lf(Left foot): Up and down flipping, 0 degrees = fully up, 90 degrees = horizontal, 180 degrees = fully down;"
            "rf(Right foot): Up and down flipping, 0 degrees = fully down, 90 degrees = horizontal, 180 degrees = fully up;"
            "lh(Left hand): Up and down swinging, 0 degrees = fully down, 90 degrees = horizontal, 180 degrees = fully up;"
            "rh(Right hand): Up and down swinging, 0 degrees = fully up, 90 degrees = horizontal, 180 degrees = fully down;"
            "sequence: Single sequence object, containing an 'a' action array, with optional top-level parameters:"
            "'d' (delay in milliseconds after the sequence is completed, used for pauses between sequences)."
            "Each action object contains:"
            "Normal mode: 's' servo position object (key names: ll/rl/lf/rf/lh/rh, values: 0-180 degrees), 'v' movement speed 100-3000 milliseconds (default 1000), 'd' action delay in milliseconds (default 0);"
            "Oscillator mode: 'osc' oscillator object, containing 'a' amplitude object (each servo amplitude 10-90 degrees, default 20 degrees), 'o' center angle object (each servo oscillation center absolute angle 0-180 degrees, default 90 degrees), 'ph' phase difference object (each servo phase difference, degrees, 0-360 degrees, default 0 degrees), 'p' period 100-3000 milliseconds (default 500), 'c' number of cycles 0.1-20.0 (default 5.0);"
            "Usage method: AI can continuously call this tool multiple times, each time sending a sequence, and the system will automatically queue and execute them in order."
            "Important note: When the left and right legs and feet are oscillating, one foot must be at 90 degrees, otherwise the robot will be damaged. If multiple sequences are sent (number of sequences > 1), when all sequences are completed and need to be reset, AI should finally call the self.otto.home tool separately for reset, and should not set the reset parameter in the sequence."
            "Example: Send 3 sequences, and finally call reset:"
            "1st call{\"sequence\":\"{\\\"a\\\":[{\\\"s\\\":{\\\"ll\\\":100},\\\"v\\\":1000}],\\\"d\\\":500}\"},"
            "2nd call{\"sequence\":\"{\\\"a\\\":[{\\\"s\\\":{\\\"ll\\\":90},\\\"v\\\":800}],\\\"d\\\":500}\"},"
            "3rd call{\"sequence\":\"{\\\"a\\\":[{\\\"s\\\":{\\\"ll\\\":80},\\\"v\\\":800}]}\"},"
            "Finally call the self.otto.home tool for reset.",
            PropertyList({Property("sequence", kPropertyTypeString,
                                   "{\"a\":[{\"s\":{\"ll\":90,\"rl\":90},\"v\":1000}]}")}),
            [this](const PropertyList &properties) -> ReturnValue
            {
                std::string sequence = properties["sequence"].value<std::string>();
                // Check if it's a JSON object (it could be a string or a parsed object).
                // If the sequence is a JSON string, use it directly; if it's an object string, you also need to use it.
                QueueServoSequence(sequence.c_str());
                return true;
            });

        // System Tools
        mcp_server.AddTool("self.otto.home", "Reset the robot to the initial position", PropertyList(),
                           [this](const PropertyList& properties) -> ReturnValue {
                               QueueAction(ACTION_HOME, 1, 1000, 1, 0);
                               return true;
                           });

        mcp_server.AddTool("self.otto.stop", "Immediately stop all actions and reset", PropertyList(),
                           [this](const PropertyList& properties) -> ReturnValue {
                               if (action_task_handle_ != nullptr) {
                                   vTaskDelete(action_task_handle_);
                                   action_task_handle_ = nullptr;
                               }
                               is_action_in_progress_ = false;
                               xQueueReset(action_queue_);

                               QueueAction(ACTION_HOME, 1, 1000, 1, 0);
                               return true;
                           });

        mcp_server.AddTool(
            "self.otto.set_trim",
            "Calibrate the position of a single servo. Set the fine-tuning parameters for a specified servo to adjust the robot's initial standing posture; the settings will be permanently saved."
            "servo_type: Servo type(left_leg/right_leg/left_foot/right_foot/left_hand/right_hand); "
            "trim_value: Fine-tuning value(-50 to 50 degrees)",
            PropertyList({Property("servo_type", kPropertyTypeString, "left_leg"),
                          Property("trim_value", kPropertyTypeInteger, 0, -50, 50)}),
            [this](const PropertyList &properties) -> ReturnValue
            {
                std::string servo_type = properties["servo_type"].value<std::string>();
                int trim_value = properties["trim_value"].value<int>();

                ESP_LOGI(TAG, "Set servo trim: %s = %d degrees", servo_type.c_str(), trim_value);

                // Get current trim values
                Settings settings("otto_trims", true);
                int left_leg = settings.GetInt("left_leg", 0);
                int right_leg = settings.GetInt("right_leg", 0);
                int left_foot = settings.GetInt("left_foot", 0);
                int right_foot = settings.GetInt("right_foot", 0);
                int left_hand = settings.GetInt("left_hand", 0);
                int right_hand = settings.GetInt("right_hand", 0);

                // Update the trim value of the specified servo
                if (servo_type == "left_leg")
                {
                    left_leg = trim_value;
                    settings.SetInt("left_leg", left_leg);
                }
                else if (servo_type == "right_leg")
                {
                    right_leg = trim_value;
                    settings.SetInt("right_leg", right_leg);
                }
                else if (servo_type == "left_foot")
                {
                    left_foot = trim_value;
                    settings.SetInt("left_foot", left_foot);
                }
                else if (servo_type == "right_foot")
                {
                    right_foot = trim_value;
                    settings.SetInt("right_foot", right_foot);
                }
                else if (servo_type == "left_hand")
                {
                    if (!has_hands_)
                    {
                        return "Error: Robot has no configured hand servos";
                    }
                    left_hand = trim_value;
                    settings.SetInt("left_hand", left_hand);
                }
                else if (servo_type == "right_hand")
                {
                    if (!has_hands_)
                    {
                        return "Error: The robot is not equipped with a hand servo motor.";
                    }
                    right_hand = trim_value;
                    settings.SetInt("right_hand", right_hand);
                }
                else
                {
                    return "Error: Invalid servo type. Please use: left_leg, right_leg, left_foot, "
                           "right_foot, left_hand, right_hand";
                }

                otto_.SetTrims(left_leg, right_leg, left_foot, right_foot, left_hand, right_hand);

                QueueAction(ACTION_JUMP, 1, 500, 0, 0);

                return "Servo " + servo_type + " trim set to " + std::to_string(trim_value) +
                       " degrees, and has been permanently saved.";
            });

        mcp_server.AddTool("self.otto.get_trims", "Get current servo trim settings", PropertyList(),
                           [this](const PropertyList& properties) -> ReturnValue {
                               Settings settings("otto_trims", false);

                               int left_leg = settings.GetInt("left_leg", 0);
                               int right_leg = settings.GetInt("right_leg", 0);
                               int left_foot = settings.GetInt("left_foot", 0);
                               int right_foot = settings.GetInt("right_foot", 0);
                               int left_hand = settings.GetInt("left_hand", 0);
                               int right_hand = settings.GetInt("right_hand", 0);

                               std::string result =
                                   "{\"left_leg\":" + std::to_string(left_leg) +
                                   ",\"right_leg\":" + std::to_string(right_leg) +
                                   ",\"left_foot\":" + std::to_string(left_foot) +
                                   ",\"right_foot\":" + std::to_string(right_foot) +
                                   ",\"left_hand\":" + std::to_string(left_hand) +
                                   ",\"right_hand\":" + std::to_string(right_hand) + "}";

                               ESP_LOGI(TAG, "Get trim settings: %s", result.c_str());
                               return result;
                           });

        mcp_server.AddTool("self.otto.get_status", "Get robot status, returns moving or idle",
                           PropertyList(), [this](const PropertyList& properties) -> ReturnValue {
                               return is_action_in_progress_ ? "moving" : "idle";
                           });

        mcp_server.AddTool("self.battery.get_level", "Get robot battery level and charging status", PropertyList(),
                           [](const PropertyList& properties) -> ReturnValue {
                               auto& board = Board::GetInstance();
                               int level = 0;
                               bool charging = false;
                               bool discharging = false;
                               board.GetBatteryLevel(level, charging, discharging);

                               std::string status =
                                   "{\"level\":" + std::to_string(level) +
                                   ",\"charging\":" + (charging ? "true" : "false") + "}";
                               return status;
                           });

        ESP_LOGI(TAG, "MCP tools registered");
    }

    ~OttoController() {
        if (action_task_handle_ != nullptr) {
            vTaskDelete(action_task_handle_);
            action_task_handle_ = nullptr;
        }
        vQueueDelete(action_queue_);
    }
};

static OttoController* g_otto_controller = nullptr;

void InitializeOttoController() {
    if (g_otto_controller == nullptr) {
        g_otto_controller = new OttoController();
        ESP_LOGI(TAG, "Otto controller initialized and MCP tools registered");
    }
}
