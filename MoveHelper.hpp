#pragma once

#include "Engine.hpp"
#include "Memory.hpp"

class MoveHelper {
public:
	MoveHelper(Soldier* soldier, uintptr_t characterMovement) {
		m_soldier = soldier;
		m_characterMovement = characterMovement;
	}

	// Enemy base: { 355036.440563, -318065.896290, 2149.079969 };
	bool Move(const Vector3& target) {
		if (!m_soldier || !m_characterMovement) {
			return true;
		}

		auto currentPosition = m_soldier->GetPosition();
		auto direction = Vector3Subtract(target, currentPosition);
		direction.z = 0.0;
		Vector3Normalize(direction);
		direction = Vector3Scale(direction, 5000.0);
		if (currentPosition.z < 10000.0) {
			direction.z = 5000.0;
		}

		MemWrite<Vector3>(m_characterMovement + 0xC0, direction);
		
		return Vector3Distance(m_soldier->GetPosition(), target) <= 10000.0;
	}

private:
	Soldier* m_soldier;
	uintptr_t m_characterMovement;
};