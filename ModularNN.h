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
	static void applyActivationFunction(float *const data, const unsigned size);

	template <ActivationFunction actFunc>
	struct StandardLayer : public Layer {
		StandardLayer(std::istream &);
		~StandardLayer();
		virtual fv run(fv &input) override;

	private:
		const unsigned inputSize, outputSize;
		float *const *const weights;
	};

	struct LSTM : public Layer {
		LSTM(std::istream &);
		~LSTM();
		virtual fv run(fv &input) override;

	private:
		// Sizes
		const unsigned inputSize, outputSize, mSize;

		// Weight matrices
		float *const *const Wmx, *const *const Wmh, *const *const Whx, *const *const Whm, *const *const Wix, *const *const Wim, *const *const Wox, *const *const Wom, *const *const Wfx, *const *const Wfm;

		// Biases
		float *const bm, *const bhr, *const bi, *const bo, *const bf, *const bc, *const bh;

		// State
		float *const hState, *const cState;
	};

	// ModularNN static functions
	static void mulMatrixVec(float const * const * const mat, float const * const vec, float * const dest, const unsigned inSize, const unsigned outSize);
	static void mulWeightsVec(float const * const * const weights, float const * const vec, float * const dest, const unsigned inSize, const unsigned outSize);
	static void addVectors(float *const target, float const *const source, const unsigned count);

	// ModularNN functions
	ModularNN(std::istream &is);
	fv run(fv &input);

private:
	std::unique_ptr<Layer> readLayer(std::istream &is);
	std::vector <std::unique_ptr<Layer>> layers;
};
