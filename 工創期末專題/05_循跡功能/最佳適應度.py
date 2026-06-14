import math
import random
import numpy as np

# ==========================================
# 1. 賽道環境設定   (手繪賽道)
# ==========================================
def generate_track():
    track = []
    # 起點直行 (往左)
    for x in range(500, 400, -5): track.append((float(x), 100.0))
    # 鈍角左彎
    for i in range(20): track.append((400.0 - i*5, 100.0 + i*5))
    # 直行 (往左)
    for x in range(300, 200, -5): track.append((float(x), 200.0))
    # 鈍角右彎
    for i in range(20): track.append((200.0 - i*5, 200.0 - i*5))
    # 直行 (往下)
    for y in range(100, 400, 5): track.append((100.0, float(y)))
    # 90度直角左彎 + 短直行
    for x in range(100, 250, 5): track.append((float(x), 400.0))
    # 90度直角左彎 + 往上直行
    for y in range(400, 250, -5): track.append((250.0, float(y)))
    # 90度直角右彎 + 直行 (閃避方塊)
    for x in range(250, 320, 5): track.append((float(x), 250.0))
    # 90度直角右彎 + 往下
    for y in range(250, 350, 5): track.append((320.0, float(y)))
    # 90度直角左彎 + 直行
    for x in range(320, 400, 5): track.append((float(x), 350.0))
    #電阻
    for i in range(20): track.append((400.0 + i*3, 350.0 - i*6)) 
    for i in range(20): track.append((460.0 + i*4, 230.0 + i*6)) 
    # 鈍角左彎回終點
    for x in range(540, 700, 5): track.append((float(x), 350.0))
    return track

TRACK = generate_track()

def get_track_error(car_x, car_y, car_angle):
    min_dist = float('inf')
    for pt in TRACK:
        dist = math.hypot(car_x - pt[0], car_y - pt[1])
        if dist < min_dist: min_dist = dist
            
    sensor_offsets = [-24, -12, 0, 12, 24]
    sensors = [0, 0, 0, 0, 0]
    
    for i, offset in enumerate(sensor_offsets):
        s_x = car_x + 20 * math.cos(car_angle) - offset * math.sin(car_angle)
        s_y = car_y + 20 * math.sin(car_angle) + offset * math.cos(car_angle)
        for pt in TRACK:
            if math.hypot(s_x - pt[0], s_y - pt[1]) < 10:
                sensors[i] = 1
                break
                
    s1, s2, s3, s4, s5 = sensors
    sum_val = 0
    active = 0
    if s1 == 1: sum_val += -39; active += 1
    if s2 == 1: sum_val += -12; active += 1
    if s3 == 1: sum_val += 0;   active += 1
    if s4 == 1: sum_val += 12;  active += 1
    if s5 == 1: sum_val += 39;  active += 1
    
    sensor_state = (s1 << 4) | (s2 << 3) | (s3 << 2) | (s4 << 1) | s5
    return sum_val, active, sensor_state

# ==========================================
# 2. 車體物理模擬 (還原鉗位與脫軌極限甩尾邏輯)
# ==========================================
def simulate_car(chromosome):
    Kp, Kd, base_speed = chromosome
    
    # 初始位置設為起點
    car_x, car_y = 500.0, 100.0
    car_angle = math.pi  # 朝左
    last_error = 0
    is_lost = False
    
    steps_survived = 0
    total_shaking = 0
    max_steps = 2500 # 增加模擬步數以跑完長賽道
    
    for step in range(max_steps):
        sum_val, active, sensor_state = get_track_error(car_x, car_y, car_angle)
        
        if sensor_state == 0b00000:
            error = -40 if last_error < 0 else 40
            is_lost = True
            if step - steps_survived > 40: break # 迷路太久淘汰
        elif sensor_state == 0b11111:
            error = -4 if last_error < 0 else 4
            is_lost = False
        elif active > 0:
            error = sum_val / active
            steps_survived = step
            is_lost = False
        else:
            error = last_error
            
        P = error
        D = error - last_error
        correction = Kp * P + Kd * D
        last_error = error
        
        curr_base_l = base_speed
        curr_base_r = base_speed
        if abs(error) >= 39:
            curr_base_l = 0
            curr_base_r = 0
            
        left_spd = curr_base_l + correction
        right_spd = curr_base_r - correction
        
        if abs(error) >= 39:
            if not is_lost:
                if left_spd < 0: left_spd = 0
                if right_spd < 0: right_spd = 0
                if left_spd > 0: left_spd = 160
                if right_spd > 0: right_spd = 160
            else:
                # V型彎脫軌大甩尾
                left_spd = 200 if last_error > 0 else -200
                right_spd = -200 if last_error > 0 else 200
            
        left_spd = max(-250, min(250, left_spd))
        right_spd = max(-250, min(250, right_spd))
        
        total_shaking += abs(D)
        
        v = (left_spd + right_spd) / 2.0 * 0.05
        w = (right_spd - left_spd) / 48.0 * 0.05
        
        car_angle += w
        car_x += v * math.cos(car_angle)
        car_y += v * math.sin(car_angle)
        
# 抵達終點 (x > 650, y 近 350) 提早結束
        if car_x > 650 and abs(car_y - 350) < 30:
            # 🏆 追求極速的計分法：完賽先給 50000 分基底！
            # step 是花費的步數（時間）。花的時間越少，(max_steps - step) 就越大！
            # 我們把省下來的時間乘上極高的權重 (100倍) 變成速度獎勵
            speed_bonus = (max_steps - step) * 100
            fitness = 50000 + speed_bonus - (total_shaking * 0.2)
            return fitness
            
    # 如果沒能跑到終點的車子（中途脫軌失敗）：
    # 乖乖按照存活步數給分，分數絕對拼不過有跑完的車
    fitness = steps_survived * 10 - (total_shaking * 0.2)
    return max(1.0, fitness)

# ==========================================
# 3. 基因演算法演進核心
# ==========================================
def genetic_algorithm():
    pop_size = 40       
    generations = 30    
    mutation_rate = 0.2 
    
    population = []
    for _ in range(pop_size):
        chromosome = [random.uniform(1.0, 5.0), random.uniform(0.5, 4.0), random.uniform(90, 190)]
        population.append(chromosome)
        
    print("====== AI 開始基因演算法優化模擬 ======")
    
    for gen in range(generations):
        fitness_scores = [simulate_car(ind) for ind in population]
        best_idx = np.argmax(fitness_scores)
        best_score = fitness_scores[best_idx]
        best_genes = population[best_idx]
        
        print(f"第 {gen+1:02d} 代 | 最佳適應度: {best_score:.1f} | Kp: {best_genes[0]:.2f}, Kd: {best_genes[1]:.2f}, 速度: {int(best_genes[2])}")
        
        total_fit = sum(fitness_scores)
        prob = [f / total_fit for f in fitness_scores]
        next_pop_indices = np.random.choice(pop_size, size=pop_size-2, p=prob)
        
        next_population = [best_genes, population[best_idx]] 
        
        for i in range(0, len(next_pop_indices), 2):
            parent1 = population[next_pop_indices[i]]
            parent2 = population[next_pop_indices[min(i+1, len(next_pop_indices)-1)]]
            
            alpha = random.random()
            child1 = [alpha * p1 + (1 - alpha) * p2 for p1, p2 in zip(parent1, parent2)]
            child2 = [(1 - alpha) * p1 + alpha * p2 for p1, p2 in zip(parent1, parent2)]
            
            for child in [child1, child2]:
                if random.random() < mutation_rate:
                    child[0] += random.gauss(0, 0.3) 
                    child[1] += random.gauss(0, 0.2) 
                    child[2] += random.gauss(0, 5)   
                    
                    child[0] = max(1.0, min(5.0, child[0]))
                    child[1] = max(0.5, min(4.0, child[1]))
                    child[2] = max(90, min(190, child[2]))
                next_population.append(child)
                
        population = next_population[:pop_size]
        
    return best_genes

if __name__ == "__main__":
    best_params = genetic_algorithm()
    print("\n======= 🎉 演算完成！最佳實車參數推薦 =======")
    print(f"請將 ESP32 內的參數修改為以下數值：")
    print(f"float Kp = {best_params[0]:.2f};")
    print(f"float Kd = {best_params[1]:.2f};")
    print(f"const int TRACK_SPEEDL = {int(best_params[2])};")
    print(f"const int TRACK_SPEEDR = {int(best_params[2])};")
    #======= 🎉 演算完成！最佳實車參數推薦 ======= 成果1(fail)
# 請將 ESP32 內的參數修改為以下數值：
# float Kp = 1.62;
# float Kd = 0.89;
# const int TRACK_SPEEDL = 115;
# const int TRACK_SPEEDR = 115;

# ======= 🎉 演算完成！最佳實車參數推薦 =======(sucess)
# 請將 ESP32 內的參數修改為以下數值：
# float Kp = 3.44;
# float Kd = 2.75;
# const int TRACK_SPEEDL = 96;
# const int TRACK_SPEEDR = 96;