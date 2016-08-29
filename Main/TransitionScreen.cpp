#include "stdafx.h"
#include "TransitionScreen.hpp"
#include "Application.hpp"
#include "Shared/Jobs.hpp"
#include "GUI.hpp"
#include "GUI/Spinner.hpp"
#include "AsyncLoadable.hpp"

class TransitionScreen_Impl : public TransitionScreen
{
	Ref<Canvas> m_loadingOverlay;
	IAsyncLoadableApplicationTickable* m_tickableToLoad;
	Job m_loadingJob;

	enum Transition
	{
		In,
		Wait,
		Out,
		End
	};
	Transition m_transition = Transition::In;
	float m_transitionTimer;

public:
	TransitionScreen_Impl(IAsyncLoadableApplicationTickable* next)
	{
		m_tickableToLoad = next;
	}
	~TransitionScreen_Impl()
	{
		// In case of forced removal of this screen
		if(!m_loadingJob->IsFinished())
			m_loadingJob->Terminate();

		g_rootCanvas->Remove(m_loadingOverlay.As<GUIElementBase>());
	}
	virtual void Tick(float deltaTime)
	{
		m_transitionTimer += deltaTime;
		
		if(m_transition == In)
		{

		}
		else if(m_transition == Out)
		{
			if(m_transitionTimer > 0.0f)
			{
				m_transition = End;
				g_application->RemoveTickable(this);
			}
		}
	}
	virtual bool Init()
	{
		if(!m_tickableToLoad)
			return false;

		m_loadingOverlay = Ref<Canvas>(new Canvas());

		// Fill screen with black
		Panel* black = new Panel();
		Canvas::Slot* blackSlot = m_loadingOverlay->Add(black->MakeShared());
		blackSlot->anchor = Anchors::Full;
		blackSlot->SetZOrder(0);
		black->color = Color::Black;

		Spinner* spinner = new Spinner(CommonGUIStyle::Get());
		Canvas::Slot* spinnerSlot = m_loadingOverlay->Add(spinner->MakeShared());
		spinnerSlot->anchor = Anchor(1.0f, 1.0f); // Right bottom corner
		spinnerSlot->padding = Margin(-50, -50, 50, 50);
		spinnerSlot->autoSizeX = true;
		spinnerSlot->autoSizeY = true;
		spinnerSlot->alignment = Vector2(1.0f, 1.0f);
		spinnerSlot->SetZOrder(1);
		
		Canvas::Slot* slot = g_rootCanvas->Add(m_loadingOverlay.As<GUIElementBase>());
		slot->anchor = Anchors::Full;
		slot->SetZOrder(1000); // Loading screen on top of all things

		m_loadingJob = JobBase::CreateLambda([&]()
		{
			return DoLoad();
		});
		m_loadingJob->OnFinished.Add(this, &TransitionScreen_Impl::OnFinished);
		g_jobSheduler->Queue(m_loadingJob);

		return true;
	}

	void OnFinished(Job job)
	{
		// Finalize?
		IAsyncLoadable* loadable = dynamic_cast<IAsyncLoadable*>(m_tickableToLoad);
		if(job->IsSuccessfull())
		{
			if(loadable && !loadable->AsyncFinalize())
			{
				Logf("[Transition] Failed to finalize loading of tickable", Logger::Error);
				delete m_tickableToLoad;
				m_tickableToLoad = nullptr;
			}
		}
		else
		{
			Logf("[Transition] Failed to load tickable", Logger::Error);
			delete m_tickableToLoad;
			m_tickableToLoad = nullptr;
		}

		if(m_tickableToLoad)
		{
			Logf("[Transition] Finished loading tickable", Logger::Info);
			g_application->AddTickable(m_tickableToLoad, this);
		}

		OnLoadingComplete.Call(m_tickableToLoad);
		m_transition = Out;
		m_transitionTimer = 0.0f;
	}
	bool DoLoad()
	{
		if(!m_tickableToLoad)
			return false;
		IAsyncLoadable* loadable = dynamic_cast<IAsyncLoadable*>(m_tickableToLoad);
		if(loadable)
		{
			if(!loadable->AsyncLoad())
			{
				Logf("[Transition] Failed to load tickable", Logger::Error);
				return false;
			}
		}
		else
		{
			if(!m_tickableToLoad->DoInit())
				return false;
		}
		return true;
	}
};

TransitionScreen* TransitionScreen::Create(IAsyncLoadableApplicationTickable* next)
{
	return new TransitionScreen_Impl(next);
}
