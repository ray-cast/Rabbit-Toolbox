#include <octoon/animator_component.h>
#include <octoon/transform_component.h>
#include <octoon/solver_component.h>
#include <octoon/game_scene.h>

namespace octoon
{
	OctoonImplementSubClass(AnimatorComponent, GameComponent, "Animator")

	AnimatorComponent::AnimatorComponent() noexcept
		: enableAnimation_(true)
		, enableAnimOnVisableOnly_(false)
		, needUpdate_(true)
		, timer_(nullptr)
		, fps_(30.0f)
		, time_(0.0f)
	{
	}

	AnimatorComponent::AnimatorComponent(animation::AnimationClip<float>&& clip) noexcept
		: AnimatorComponent()
	{
		this->clips_.emplace_back(clip);
	}

	AnimatorComponent::AnimatorComponent(animation::AnimationClips<float>&& clips) noexcept
		: AnimatorComponent()
	{
		this->clips_ = std::move(clips);
	}

	AnimatorComponent::AnimatorComponent(const animation::AnimationClip<float>& clip) noexcept
		: AnimatorComponent()
	{
		this->clips_.emplace_back(clip);
	}

	AnimatorComponent::AnimatorComponent(const animation::AnimationClips<float>& clips) noexcept
		: AnimatorComponent()
	{
		this->clips_ = clips;
	}

	AnimatorComponent::AnimatorComponent(GameObjects&& transforms) noexcept
		: AnimatorComponent()
	{
		this->setTransforms(std::move(transforms));
	}

	AnimatorComponent::AnimatorComponent(const GameObjects& transforms) noexcept
		: AnimatorComponent()
	{
		this->setTransforms(transforms);
	}

	AnimatorComponent::~AnimatorComponent() noexcept
	{
	}

	bool
	AnimatorComponent::play(const std::string& filename) noexcept
	{
		this->setName(filename);
		this->addComponentDispatch(GameDispatchType::FrameEnd);

		enableAnimation_ = true;
		return enableAnimation_;
	}

	void
	AnimatorComponent::pause() noexcept
	{
		enableAnimation_ = false;
	}

	void
	AnimatorComponent::stop() noexcept
	{
	}

	void
	AnimatorComponent::setClips(animation::AnimationClips<float>&& clips) noexcept
	{
		clips_ = std::move(clips);
	}

	void
	AnimatorComponent::setClips(const animation::AnimationClips<float>& clips) noexcept
	{
		clips_ = clips;
	}

	const animation::AnimationClips<float>&
	AnimatorComponent::getClips() const noexcept
	{
		return clips_;
	}

	void
	AnimatorComponent::setTransforms(GameObjects&& transforms) noexcept
	{
		bones_ = std::move(transforms);
		this->updateBindpose(bones_);
	}

	void
	AnimatorComponent::setTransforms(const GameObjects& transforms) noexcept
	{
		bones_ = transforms;
		this->updateBindpose(bones_);
	}

	const GameObjects&
	AnimatorComponent::getTransforms() const noexcept
	{
		return bones_;
	}

	GameComponentPtr
	AnimatorComponent::clone() const noexcept
	{
		auto animtion = std::make_shared<AnimatorComponent>();
		animtion->bones_ = bones_;
		return animtion;
	}

	void 
	AnimatorComponent::onActivate() except
	{
		timer_ = this->getGameObject()->getGameScene()->getFeature<TimerFeature>();
	}

	void
	AnimatorComponent::onDeactivate() noexcept
	{
		timer_ = nullptr;
		this->removeComponentDispatch(GameDispatchType::FrameEnd);
	}

	void
	AnimatorComponent::onFrameEnd() noexcept
	{
		time_ += timer_->delta();

		if (time_ > (1.0f / fps_))
		{
			needUpdate_ = true;
			time_ -= (1.0f / fps_);
		}

		if (needUpdate_)
		{
			for (auto& it : clips_)
				it.evaluate(1.0f / fps_);

			this->updateBones();
			this->updateSolvers();
			this->sendMessageDownwards("octoon:animation:update");

			needUpdate_ = false;
		}
	}

	void
	AnimatorComponent::updateBindpose(const GameObjects& bones) noexcept
	{
		bindpose_.resize(bones.size());

		for (std::size_t i = 0; i < bones.size(); i++)
		{
			bindpose_[i] = bones[i]->getComponent<TransformComponent>()->getTranslate();
			if (bones[i]->getParent())
				bindpose_[i] -= bones[i]->getParent()->getComponent<TransformComponent>()->getTranslate();
		}
	}

	void
	AnimatorComponent::updateBones() noexcept
	{
		for (std::size_t i = 0; i < clips_.size(); i++)
		{
			assert(bones_[i]->getName() == clips_[i].name);

			auto transform = bones_[i]->getComponent<TransformComponent>();
			auto translate = transform->getLocalTranslate();
			auto quat = transform->getLocalQuaternion();

			for (auto& curve : clips_[i].curves)
			{
				if (curve.first == "LocalPosition.x")
					translate.x = curve.second.key.value + bindpose_[i].x;
				else if (curve.first == "LocalPosition.y")
					translate.y = curve.second.key.value + bindpose_[i].y;
				else if (curve.first == "LocalPosition.z")
					translate.z = curve.second.key.value + bindpose_[i].z;
				else if (curve.first == "LocalQuaternion.x")
					quat.x = curve.second.key.value;
				else if (curve.first == "LocalQuaternion.y")
					quat.y = curve.second.key.value;
				else if (curve.first == "LocalQuaternion.z")
					quat.z = curve.second.key.value;
				else if (curve.first == "LocalQuaternion.w")
					quat.w = curve.second.key.value;
			}

			transform->setLocalTranslate(translate);
			transform->setLocalQuaternion(math::normalize(quat));
		}
	}

	void
	AnimatorComponent::updateSolvers() noexcept
	{
		for (auto& it : bones_)
		{
			auto solver = it->getComponent<SolverComponent>();
			if (solver)
				solver->solver();
		}
	}
}