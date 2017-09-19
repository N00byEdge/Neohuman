#pragma once

#include <vector>
#include <iostream>
#include <memory>

using fv = std::vector <float>;

struct ModularNN {
	// NN related definitions
	struct Layer {
		virtual fv run(fv &input) = 0;
	};

	enum class ActivationFunction {
		ReLU,
		Sigmoid,
		Tanh,
		Softmax,
	};

	template <ActivationFunction actFunc>
	static void applyActivationFunction(fv &v);

	template <ActivationFunction actFunc>
	struct StandardLayer : public Layer {
		StandardLayer(std::istream &);
		virtual fv run(fv &input) override;

		const unsigned inputSize, outputSize;
		const std::unique_ptr<float[][]> weights;
	private:
	};

	// ModularNN static functions
	static void mulWeightsVec(float const **weights, float const *vec, float *dest, const unsigned inSize, const unsigned outSize);

	// ModularNN functions
	ModularNN(std::istream &is);
	fv run(fv &input);

private:
	std::unique_ptr<Layer> readLayer(std::istream &is);
	std::vector <std::unique_ptr<Layer>> layers;
};
