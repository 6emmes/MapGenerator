#include "rivers.h"


inline bool outOfBounds(int x, int y, int width, int height, int margin=0) {
    return (x < margin || x >= width - margin || y < margin || y >= height - margin);
}

std::pair<int, float> createRiver(int width, int height, const std::pair<int,int> &originPoint, std::vector<std::vector<float>> &riverMap,
    std::vector<std::vector<float>> &heightmap, std::vector<std::vector<float>> &waterMap) {
    std::pair<int,int> nextstep;
    std::pair<int,int> currentPoint = originPoint;
    std::vector<std::pair<int,int>> directions = {{1,0}, {-1,0}, {0,1}, {0,-1}};
    float curheight = 0.0;
    //=========================================================move closer to the peak
    for(int loo=0;loo<40;loo++){
        if (outOfBounds(currentPoint.first, currentPoint.second, width, height, 1)) break;
        curheight = heightmap[currentPoint.second][currentPoint.first];
        float nextheight = 0;
        for(int i=-1;i<=1;i++){
            for(int j=-1;j<=1;j++){
                if(i==0 && j==0) continue;
                if(i==j || i==-j) continue;
                if (outOfBounds(currentPoint.first + i, currentPoint.second + j, width, height, 1)) continue;
                float nh = heightmap[currentPoint.second + j][currentPoint.first + i];
                if (nh > nextheight) {
                    nextheight = nh;
                    nextstep = std::make_pair(i,j);
                }
            }
        }
        if (nextheight <= curheight) break; // reached local
        currentPoint.first += nextstep.first;
        currentPoint.second += nextstep.second;
    }
    currentPoint.first = (originPoint.first+currentPoint.first*2)/3;
    currentPoint.second = (originPoint.second+currentPoint.second*2)/3;
    for (int i=-10;i<=10;i++){
        for (int j=-10;j<=10;j++){
            if (outOfBounds(currentPoint.first + i, currentPoint.second + j, width, height)) continue;
            if (riverMap[currentPoint.second + j][currentPoint.first + i] > 0.2 && 
                (currentPoint.second + j!=originPoint.second || currentPoint.first + i!=originPoint.first)) {
                //std::cout << "River killed" << std::endl;
                return {0,0};
            }
        }
    }
    std::queue<std::pair<int,int>> tmpPathFrontier;
    std::vector<std::vector<float>> tmpMap(height, std::vector<float>(width, 0.0f));
    std::pair<int,int> minHeightPoint = currentPoint;
    std::pair<int,int> loopStart = originPoint;
    std::pair<int,int> riverEnd;
    float value = 0.0f;
    float delta = 0.001f;
    tmpMap[currentPoint.second][currentPoint.first] = value;
    tmpPathFrontier.emplace(currentPoint);
    curheight = heightmap[currentPoint.second][currentPoint.first]+0.001;
    int riverlen = 0;
    //=====================================================================Forward search
    for (int i = 0;i<5000;i++){
        if (i == 4999)return {0,0};
        if (tmpPathFrontier.size() == 0) {
            //std::cout<<"River dead end reached"<<std::endl;
            return {0,0};
        }
        currentPoint = tmpPathFrontier.front();
        tmpPathFrontier.pop();
        if (heightmap[currentPoint.second][currentPoint.first] < curheight) {
            curheight = heightmap[currentPoint.second][currentPoint.first];
            curheight += curheight/200;
            minHeightPoint = currentPoint;
        }
        if (heightmap[currentPoint.second][currentPoint.first] == 0) {
            //std::cout<<"River reached sea"<<std::endl;
            break;
        }
        if (outOfBounds(currentPoint.second, currentPoint.first, width, height, 1)) continue;
        value = tmpMap[currentPoint.second][currentPoint.first];
        for (const auto& d : directions) {
            if (tmpMap[currentPoint.second + d.second][currentPoint.first + d.first] > 0)   continue;
            if (heightmap[currentPoint.second + d.second][currentPoint.first + d.first] < curheight){
                tmpPathFrontier.emplace(currentPoint.first + d.first, currentPoint.second + d.second);
                tmpMap[currentPoint.second + d.second][currentPoint.first + d.first] = value+curheight-heightmap[currentPoint.second + d.second][currentPoint.first + d.first];
                riverlen++;
            }
        }
    }

    std::vector<std::pair<int,int>> neighbors;
    uint16_t index = 0;
    std::stack<std::pair<int,int>> tmpPath;
    std::pair<int,int> checkPoint = {-1,-1};
    //=====================================================================Reverse trace
    int debug = 0;
    for (;;){
        if (tmpPath.size()%10==0){
            if (currentPoint == checkPoint) {
                //std::cout<<"River stuck in loop"<<std::endl;
                return {0,0};
            }
            checkPoint = currentPoint;
        }

        if (debug>1000) break;
        debug++;
        tmpPath.emplace(currentPoint);
        if (tmpMap[currentPoint.second][currentPoint.first] <= delta) break;
        float curscore = tmpMap[currentPoint.second][currentPoint.first];
        neighbors = {};
        for (const auto& d : directions) {
            if (tmpMap[currentPoint.second + d.second][currentPoint.first + d.first] <= curscore && tmpMap[currentPoint.second + d.second][currentPoint.first + d.first]>0) {
                neighbors.emplace_back(currentPoint.first + d.first, currentPoint.second + d.second);
            }
        }
        if (neighbors.size() == 0) break;
        index = index * 37 + 13;
        currentPoint = neighbors[index%neighbors.size()];
    }
    value = 0.02;
    int tmpPathSize = tmpPath.size();
    int riverFound = 0;
    float riverHash = 0;
    //=====================================================================Saving the path to the map
    for (int j=0;j<tmpPathSize;j++) {
        currentPoint = tmpPath.top();
        tmpPath.pop();
        if (riverMap[currentPoint.second][currentPoint.first] > 0) {
            riverFound = 1;
            break;
        }
        else riverMap[currentPoint.second][currentPoint.first] = value;
        riverHash += value;
        value += delta;
    }
    return {1, riverHash};
}

std::vector<std::vector<float>> generateRiverPoints_main(int width, int height,
    std::vector<std::vector<float>> &heightmap,
    std::vector<std::vector<float>> &landMap) {
    std::vector<std::vector<float>> riverMap(height, std::vector<float>(width, 0.0f));
    std::mt19937 rng(3);
    std::uniform_int_distribution<int> distX(0, static_cast<int>(width - 1));
    std::uniform_int_distribution<int> distY(0, static_cast<int>(height - 1));

    std::vector<std::pair<int,int>> points;
    points.reserve(static_cast<size_t>(RIVER_COUNT));
    for (int i = 0; i < RIVER_COUNT; ++i) {
        int x = distX(rng);
        int y = distY(rng);
        if (heightmap[y][x]*MAXALTITUDE > 1000) {
            points.emplace_back(x, y);
        }
    }
    int riversmade = 0;
    std::pair<int, float> tmp;
    int riverHash = 0;
    for (const auto &p : points) {
        tmp = createRiver(width, height, p, riverMap, heightmap, landMap);
        riversmade += tmp.first;
        riverHash += int(tmp.second*100);
    }
    std::cout << "Generated " << riversmade <<" rivers out of "<< points.size() << " starting river points in " << RIVER_COUNT << " attempts.\n";
    std::cout <<"River hash: "<< riverHash <<std::endl;
    return riverMap;
}