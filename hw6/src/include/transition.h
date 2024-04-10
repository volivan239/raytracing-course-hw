#pragma once
#include "vec3.h"
#include "quaternion.h"
#include <stddef.h>
#include <cstring>

class Transition{
private:
    float matrix_[4][4];
    
    static Transition fromScale(const Vec3 &scale) {
        float matrix[4][4] = {
            {scale.x, 0,       0,       0},
            {0,       scale.y, 0,       0},
            {0,       0,       scale.z, 0},
            {0,       0,       0,       1}
        };
        return Transition(matrix);
    }

    static Transition fromTranslation(const Vec3 &translation) {
        float matrix[4][4] = {
            {1, 0, 0, translation.x},
            {0, 1, 0, translation.y},
            {0, 0, 1, translation.z},
            {0, 0, 0, 1            }
        };
        return Transition(matrix);
    }

    static Transition fromRotation(const Quaternion &q) {
        float matrix[4][4] = {
            {2 * (q.w * q.w + q.v.x * q.v.x) - 1, 2 * (q.v.x * q.v.y - q.w * q.v.z),   2 * (q.v.x * q.v.z + q.w * q.v.y),   0},
            {2 * (q.v.x * q.v.y + q.w * q.v.z),   2 * (q.w * q.w + q.v.y * q.v.y) - 1, 2 * (q.v.y * q.v.z - q.w * q.v.x),   0},
            {2 * (q.v.x * q.v.z - q.w * q.v.y),   2 * (q.v.y * q.v.z + q.w * q.v.x),   2 * (q.w * q.w + q.v.z * q.v.z) - 1, 0},
            {0,                                   0,                                   0,                                   1}
        };
        return Transition(matrix);
    }

public:
    Transition() {
        memset(matrix_, 0, sizeof(matrix_));
        matrix_[0][0] = matrix_[1][1] = matrix_[2][2] = matrix_[3][3] = 1;
    }

    Transition(float matrix[4][4]) {
        memcpy(matrix_, matrix, sizeof(matrix_));
    }

    Transition(const Vec3 &translation, const Quaternion &rotation, const Vec3 &scale) {
        *this = fromTranslation(translation).compose(fromRotation(rotation)).compose(fromScale(scale));
    }

    Transition compose(const Transition &other) const {
        Transition result;
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                for (int k = 0; k < 4; k++) {
                    result.matrix_[i][j] += matrix_[i][k] * other.matrix_[k][j];
                }
            }
        }
        return result;
    }

    Vec3 apply(const Vec3 &p) const {
        float result[3];
        for (int i = 0; i < 3; i++) {
            result[i] = matrix_[i][0] * p.x + matrix_[i][1] * p.y + matrix_[i][2] * p.z + matrix_[i][3];
        }
        return {result[0], result[1], result[2]};
    }
};