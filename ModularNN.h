#pragma once

#include <vector>
#include <iostream>
#include <memory>

using fv = std::vector <float>;

constexpr float I = 0.1f;

struct ModularNN {
	// NN related definitions
	struct Layer {
		virtual fv run(fv &input) = 0;
		virtual void write(std::ostream &) = 0;
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
		StandardLayer(unsigned inputSize, unsigned outputSize);
		StandardLayer(std::istream &);
		~StandardLayer();
		virtual fv run(fv &input) override;
		virtual void write(std::ostream &) override;

	private:
		void genWeights();
		void writeWeights(std::ostream &);
		const unsigned inputSize, outputSize;
		float *const weights;
	};

	struct LSTM : public Layer {
		LSTM(unsigned inputSize, unsigned outputSize, unsigned mSize);
		LSTM(std::istream &);
		~LSTM();
		virtual fv run(fv &input) override;
		virtual void write(std::ostream &) override;

	private:
		// Sizes
		const unsigned inputSize, outputSize, mSize;

		// Weight matrices
		float *const Wmx, *const Wmh, *const Whx, *const Whm, *const Wix, *const Wim, *const Wox, *const Wom, *const Wfx, *const Wfm;

		// Biases
		float *const bm, *const bhr, *const bi, *const bo, *const bf, *const bc, *const bh;

		// State
		float *const hState, *const cState;
	};

	// ModularNN static functions
	static void mulMatrixVec(float const *const mat, float const *const vec, float *const dest, const unsigned inSize, const unsigned outSize);
	static void mulWeightsVec(float const *const weights, float const *const vec, float *const dest, const unsigned inSize, const unsigned outSize);
	static void addVectors(float *const target, float const *const source, const unsigned count);
	template <ActivationFunction>
	static void initWeightMatrix(float *const weights, unsigned inputSize, unsigned outputSize);
	template <ActivationFunction>
	static void initWeightVector(float *const weights, unsigned inputSize, unsigned outputSize);

	// ModularNN functions
	ModularNN(std::vector <std::unique_ptr<Layer>> &layers);
	ModularNN(std::istream &is);
	fv run(fv input);
	void write(std::ostream &os);
	ModularNN genDivNN();

private:
	std::unique_ptr<Layer> readLayer(std::istream &is);
	std::vector <std::unique_ptr<Layer>> layers;
};
