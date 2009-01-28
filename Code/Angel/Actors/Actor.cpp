#include "../Actors/Actor.h"

#include "../Infrastructure/TagCollection.h"
#include "../Infrastructure/Textures.h"
#include "../Infrastructure/TextRendering.h"

#include "../Actors/ActorFactory.h"
#include "../Infrastructure/DeveloperConsole.h"
#include "../Util/StringUtil.h"
#include "../Util/MathUtil.h"
#include "../Messaging/Switchboard.h"
#include "../Infrastructure/World.h"

#include <sstream>

std::map<String, Actor*> Actor::_nameList;

Actor::Actor()
{
	SetColor(1.0f, 1.0f, 1.0f);
	SetAlpha(1.0f);
	SetSize(1.0f);
	SetRotation(0.0f);
	SetPosition(0.0f, 0.0f);
	SetUVs(Vector2(0.f, 0.f), Vector2(1.f, 1.f));
	SetName("");

	_spriteNumFrames = 0;
	_spriteCurrentFrame = 0;
	_spriteTextureReferences[0] = -1; 
	_spriteFrameDelay = 0.0f;

	_layer = 0;

    _drawShape = ADS_Square;
}

Actor::~Actor()
{
	StringSet::iterator it = _tags.begin();
	while (it != _tags.end())
	{
		String tag = *it;
		it++;
		Untag(tag);
	}

	Actor::_nameList.erase(_name);
	
	StringSet subs = theSwitchboard.GetSubscriptionsFor(this);
	it = subs.begin();
	while (it != subs.end())
	{
		theSwitchboard.UnsubscribeFrom(this, *it);
		++it;
	}
}

void Actor::Update(float dt)
{
	UpdateSpriteAnimation(dt);
	
	if (_positionInterval.ShouldStep())
	{
		SetPosition(_positionInterval.Step(dt));
		if (!_positionInterval.ShouldStep())
		{
			if (_positionIntervalMessage != "")
			{
				theSwitchboard.Broadcast(new Message(_positionIntervalMessage, this));
			}
		}
	}
	if (_rotationInterval.ShouldStep())
	{
		SetRotation(_rotationInterval.Step(dt));
		if (!_rotationInterval.ShouldStep())
		{
			if (_rotationIntervalMessage != "")
			{
				theSwitchboard.Broadcast(new Message(_rotationIntervalMessage, this));
			}
		}
	}
	if (_colorInterval.ShouldStep())
	{
		SetColor(_colorInterval.Step(dt));
		if (!_colorInterval.ShouldStep())
		{
			if (_colorIntervalMessage != "")
			{
				theSwitchboard.Broadcast(new Message(_colorIntervalMessage, this));
			}
		}
	}
	if (_sizeInterval.ShouldStep())
	{
		Vector2 newSize = _sizeInterval.Step(dt);
		SetSize(newSize.X, newSize.Y);
		if (!_sizeInterval.ShouldStep())
		{
			if (_sizeIntervalMessage != "")
			{
				theSwitchboard.Broadcast(new Message(_sizeIntervalMessage, this));
			}
		}
	}
}

void Actor::OnCollision(Actor* /*other*/)
{
}

void Actor::UpdateSpriteAnimation(float dt)
{
	if (_spriteFrameDelay > 0.0f)
	{
		_spriteCurrentFrameDelay -= dt;

		if (_spriteCurrentFrameDelay < 0.0f)
		{
			while (_spriteCurrentFrameDelay < 0.0f)
			{
				if (_spriteAnimType == SAT_Loop)
				{
					if (_spriteCurrentFrame == _spriteAnimEndFrame)
						_spriteCurrentFrame = _spriteAnimStartFrame;
					else
						++_spriteCurrentFrame;
				}
				else if (_spriteAnimType == SAT_PingPong)
				{
					if (_spriteAnimDirection == 1)
					{
						if (_spriteCurrentFrame == _spriteAnimEndFrame)
						{
							_spriteAnimDirection = -1;
							_spriteCurrentFrame = _spriteAnimEndFrame - 1;
						}
						else
							++_spriteCurrentFrame;

					}
					else
					{
						if (_spriteCurrentFrame == _spriteAnimStartFrame)
						{
							_spriteAnimDirection = 1;
							_spriteCurrentFrame = _spriteAnimStartFrame + 1;
						}
						else 
						{
							--_spriteCurrentFrame; 
						}
					}
				}
				else if (_spriteAnimType == SAT_OneShot)
				{
					// If we're done with our one shot and they set an animName, let them know it's done.
					if (_spriteCurrentFrame == _spriteAnimEndFrame)
					{
						// Needs to get called before callback, in case they start a new animation.
						_spriteAnimType = SAT_None;

						if (_currentAnimName.length() > 0) 
						{
							AnimCallback(_currentAnimName);
						}
					}
					else
					{
						_spriteCurrentFrame += _spriteAnimDirection;
					}
				}

				_spriteCurrentFrameDelay += _spriteFrameDelay;
			}
		}
	}
}

void Actor::SetDrawShape( actorDrawShape drawShape )
{
	_drawShape = drawShape;
}

const actorDrawShape Actor::GetDrawShape()
{
	return _drawShape;
}

void Actor::MoveTo(Vector2 newPosition, float duration, String onCompletionMessage)
{
	_positionInterval = Interval<Vector2>(_position, newPosition, duration);
	_positionIntervalMessage = onCompletionMessage;
}

void Actor::RotateTo(float newRotation, float duration, String onCompletionMessage)
{
	_rotationInterval = Interval<float>(_rotation, newRotation, duration);
	_rotationIntervalMessage = onCompletionMessage;
}

void Actor::ChangeColorTo(Color newColor, float duration, String onCompletionMessage)
{
	_colorInterval = Interval<Color>(_color, newColor, duration);
	_colorIntervalMessage = onCompletionMessage;
}

void Actor::ChangeSizeTo(Vector2 newSize, float duration, String onCompletionMessage)
{
	_sizeInterval = Interval<Vector2>(_size, newSize, duration);
	_sizeIntervalMessage = onCompletionMessage;
}

void Actor::ChangeSizeTo(float newSize, float duration, String onCompletionMessage)
{
	ChangeSizeTo(Vector2(newSize, newSize), duration, onCompletionMessage);
}

void Actor::Render()
{
	glPushMatrix();
	glTranslatef(_position.X, _position.Y, 0.0f);
	glRotatef(_rotation, 0, 0, 1);
	glScalef(_size.X, _size.Y, 1.0f);
	glColor4f(_color.R, _color.G, _color.B, _color.A);

	int textureReference = _spriteTextureReferences[_spriteCurrentFrame];
	if (textureReference >= 0)
	{
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, textureReference);
	}

    switch( _drawShape )
    {
        default:
        case ADS_Square:
	        glBegin(GL_QUADS);
		        //glNormal3f(0.0f, 0.0f, 1.0f);
		        glTexCoord2f(UV_rightup.X, UV_rightup.Y); glVertex2f( 0.5f,  0.5f);
		        glTexCoord2f(UV_leftlow.X, UV_rightup.Y); glVertex2f(-0.5f,  0.5f);
		        glTexCoord2f(UV_leftlow.X, UV_leftlow.Y); glVertex2f(-0.5f, -0.5f);
		        glTexCoord2f(UV_rightup.X, UV_leftlow.Y); glVertex2f( 0.5f, -0.5f);
	        glEnd();
        break;

        case ADS_Circle:
            const int NUM_SECTIONS = 32;
            glBegin(GL_TRIANGLE_FAN);
            glVertex2f(0, 0);
            for (float i = 0; i <= NUM_SECTIONS; i++)
                glVertex2f(0.5f*cos((float) MathUtil::TwoPi * i / NUM_SECTIONS), 0.5f*sin((float) MathUtil::TwoPi * i / NUM_SECTIONS));
            glEnd();
        break;
    }

	if (textureReference >= 0)
	{
		glDisable(GL_TEXTURE_2D);
	}

	glPopMatrix();
}

void Actor::SetSize(float x, float y)
{
	float sizeX, sizeY;
	if (x < 0.0f)
		sizeX = 0.0f;
	else
		sizeX = x;
	if (y <= 0.f)
		sizeY = x;
	else
		sizeY = y;
	_size = Vector2(sizeX, sizeY);
}

void Actor::SetSize(Vector2 newSize)
{
	if (newSize.X < 0.0f)
	{
		newSize.X = 0.0f;
	}
	if (newSize.Y < 0.0f)
	{
		newSize.Y = 0.0f;
	}
	_size = newSize;
}

const Vector2 Actor::GetSize()
{
	return _size;
}

void Actor::SetPosition(float x, float y)
{
	_position.X = x;
	_position.Y = y;
}

void Actor::SetPosition(Vector2 pos)
{
	_position = pos;
}

const Vector2 Actor::GetPosition()
{
	return _position;
}

void Actor::SetRotation(float rotation)
{
	_rotation = rotation;
}

const float Actor::GetRotation()
{
	return _rotation;
}

const Color Actor::GetColor()
{
	return _color;
}

void Actor::SetColor(float r, float g, float b, float a)
{
	_color = Color(r, g, b, a);
}

void Actor::SetColor(Color color)
{
	_color = color;
}

void Actor::SetAlpha(float newAlpha)
{
	_color.A = newAlpha;
}

const float Actor::GetAlpha()
{
	return _color.A;
}

void Actor::SetSpriteTexture(int texRef, int frame)
{
	frame = MathUtil::Clamp(frame, 0, MAX_SPRITE_FRAMES - 1);

	// Make sure to bump the number of frames if this frame surpasses it.
	if (frame >= _spriteNumFrames)
	{
		_spriteNumFrames = frame + 1;
	}

	_spriteTextureReferences[frame] = texRef;
}

const int Actor::GetSpriteTexture(int frame)
{
	frame = MathUtil::Clamp(frame, 0, _spriteNumFrames - 1);

	return _spriteTextureReferences[frame];
}


// Will load the sprite if it doesn't find it in the texture cache.
// The texture cache caches textures by filename.
bool Actor::SetSprite(String filename, int frame, GLint clampmode, GLint filtermode, bool optional)
{
	int textureReference = GetTextureReference(filename, clampmode, filtermode, optional);
	if (textureReference == -1)
		return false;

	SetSpriteTexture(textureReference, frame);
	return true;
}

void Actor::ClearSpriteInfo()
{
	for (int i=0; i<_spriteNumFrames; ++i)
	{
		_spriteTextureReferences[i] = -1;
	}
	_spriteAnimType = SAT_None;
	_spriteFrameDelay = 0.0f;
	_spriteCurrentFrame = 0;
}

void Actor::SetSpriteFrame(int frame)
{
	frame = MathUtil::Clamp(frame, 0, _spriteNumFrames - 1);

	if (_spriteTextureReferences[frame] == -1)
	{
		std::cout << "setSpriteFrame() - Warning: frame(" << frame << ") has an invalid texture reference." << std::endl;
	}

	_spriteCurrentFrame = frame;
}

void Actor::PlaySpriteAnimation(float delay, spriteAnimationType animType, int startFrame, int endFrame, const char* _animName)
{
	startFrame = MathUtil::Clamp(startFrame, 0, _spriteNumFrames-1);
	endFrame = MathUtil::Clamp(endFrame, 0, _spriteNumFrames-1);

	_spriteAnimDirection = startFrame > endFrame ? -1 : 1;

	_spriteCurrentFrameDelay = _spriteFrameDelay = delay;
	_spriteAnimType= animType;
	_spriteAnimStartFrame = _spriteCurrentFrame = startFrame;
	_spriteAnimEndFrame = endFrame;
	if (_animName)
		_currentAnimName = _animName;
}

/*-----------------------------------------------------------------------------

	Actor::LoadSpriteFrames()

	How this works:
		We expect the name of the first image to end in _###. 
		The number of digits doesn't matter, but internally, we are limited 
		to 64 frames.  To change that limit, just change MAX_SPRITE_FRAMES 
		in Actor.h.

-----------------------------------------------------------------------------*/
void Actor::LoadSpriteFrames(String firstFilename, GLint clampmode, GLint filtermode)
{
	int extensionLocation = firstFilename.rfind(".");
	int numberSeparator = firstFilename.rfind("_");
	int numDigits = extensionLocation - numberSeparator - 1;

	// Clear out the number of frames we think we have.
	_spriteNumFrames = 0;

	bool bValidNumber = true;
	// So you're saying I've got a chance?
	if (numberSeparator > 0 && numDigits > 0)
	{
		// Now see if all of the digits between _ and . are numbers (i.e. test_001.jpg).
		for (int i=1; i<=numDigits; ++i)
		{
			char digit = firstFilename[numberSeparator+i];
			if (digit < '0' || digit > '9')
			{
				bValidNumber = false;
				break;
			}
		}
	}

	// If these aren't valid, the format is incorrect.
	if (numberSeparator == (int)String::npos || numDigits <= 0 || !bValidNumber)
	{
		std::cout << "LoadSpriteFrames() - Bad Format - Expecting somename_###.ext" << std::endl;
		std::cout << "Attempting to load single texture: " << firstFilename << std::endl;

		if (!SetSprite(firstFilename, 0, clampmode, filtermode))
			return;
	}

	// If we got this far, the filename format is correct.
	String numberString;
	// The number string is just the digits between the '_' and the file extension (i.e. 001).
	numberString.append(firstFilename.c_str(), numberSeparator+1, numDigits);

	// Get our starting numberical value.
	int number = atoi(numberString.c_str());

	String baseFilename;
	// The base name is everything up to the '_' before the number (i.e. somefile_).
	baseFilename.append( firstFilename.c_str(), numberSeparator+1);

	String extension;
	// The extension is everything after the number (i.e. .jpg).
	extension.append(firstFilename.c_str(), extensionLocation, firstFilename.length() - extensionLocation);

	// Keep loading until we stop finding images in the sequence.
	while (true)
	{
		// Build up the filename of the current image in the sequence.
		String newFilename = baseFilename + numberString + extension;
		
		// Were we able to load the file for this sprite?
		if (!SetSprite(newFilename, _spriteNumFrames, clampmode, filtermode, true /*optional*/))
		{
			break;
		}

		// Verify we don't go out of range on our hard-coded frame limit per sprite.
		if (_spriteNumFrames >= MAX_SPRITE_FRAMES)
		{
			std::cout << "Maximum number of frames reached (" << MAX_SPRITE_FRAMES << ").  Bailing out...  \n";
			std::cout << "Increment MAX_SPRITE_FRAMES if you need more.\n\n";
			break;
		}

		// Bump the number to the next value in the sequence.
		++number;

		// Serialize the numerical value to it so we can retrieve the string equivalent.
		std::stringstream sstr;
		sstr << number;
		String newNumberString = sstr.str();

		// We assume that all the files have as many numerical digits as the first one (or greater) (i.e. 01..999).
		// See if we need to pad with leading zeros.
		int numLeadingZeros = numDigits - (int)newNumberString.length();

		// Do the leading zero padding.
		for (int i=0; i<numLeadingZeros; ++i)
		{
			newNumberString = '0' + newNumberString;
		}

		// Save off the newly formulated number string for the next image in the sequence.
		numberString = newNumberString;
	}
}

void Actor::SetUVs(const Vector2 lowleft, const Vector2 upright)
{
	UV_rightup = upright;
	UV_leftlow = lowleft;
}

void Actor::GetUVs(Vector2 &lowleft, Vector2 &upright) const
{
	upright = UV_rightup;
	lowleft = UV_leftlow;
}

const bool Actor::IsTagged(String tag)
{
	StringSet::iterator it = _tags.find(tag);
	if (it != _tags.end())
	{
		return true;
	}
	else
	{
		return false;
	}
}

void Actor::Tag(String newTag)
{
	StringList tags = SplitString(newTag, ", ");
	for(unsigned int i=0; i < tags.size(); i++)
	{
		tags[i] = ToLower(tags[i]);
		_tags.insert(tags[i]);
		theTagList.AddObjToTagList(this, tags[i]);
	}
}

void Actor::Untag(String oldTag)
{
	_tags.erase(oldTag);
	theTagList.RemoveObjFromTagList(this, oldTag);
}

const StringSet Actor::GetTags()
{
	return _tags;
}

const String Actor::SetName(String newName)
{
	if(newName.length() == 0)
	{
		newName = "Actor";
	}

	newName[0] = toupper(newName[0]);

	if (Actor::GetNamed(newName) == NULL)
	{
		_name = newName;
	}
	else
	{
		int counter = 1;
		std::ostringstream iteratedName;
		do 
		{
			iteratedName.str("");
			iteratedName << newName << counter++;
		} while(Actor::GetNamed(iteratedName.str()) != NULL);

		_name = iteratedName.str();
	}

	Actor::_nameList[_name] = this;

	return _name;
}

const String Actor::GetName()
{
	return _name;
}

const Actor* Actor::GetNamed(String nameLookup)
{
	std::map<String,Actor*>::iterator it = _nameList.find(nameLookup);
	if (it == _nameList.end())
	{
		return NULL;
	}
	else
	{
		return it->second;
	}
}

Actor* Actor::Create(String archetype)
{
	//TODO: this is kind of a fragile way to get the Actor the script created.
	//  Let's figure out a smarter way to get the object directly from Python, 
	//  hopefully without introducing a bunch of Python API code into Angel proper. 
	
	String markerTag = "last-spawned-actor-from-code";
	String toExec = "Actor._lastSpawnedActor = Actor.Create('" + archetype + "')\n";
	toExec += "Actor._lastSpawnedActor.Tag('" + markerTag + "')\n";
	theWorld.ScriptExec(toExec);
	ActorSet tagged = theTagList.GetObjectsTagged(markerTag);
	if (tagged.size() > 1)
	{
		std::cout << "WARNING: more than one Actor tagged with '" + markerTag + "'. (" 
						+ archetype + ")" << std::endl;
	}
	if (tagged.size() < 1)
	{
		std::cout << "ERROR: Script failed to create Actor with archetype " + archetype + ". " << std::endl;
		return NULL;
	}
	ActorSet::iterator it = tagged.begin();
	Actor* forReturn = *it;
	toExec = "Actor._lastSpawnedActor.__disown__()\n";
	toExec += "Actor._lastSpawnedActor = None\n";
	theWorld.ScriptExec(toExec);
	if (forReturn != NULL)
	{
		forReturn->Untag(markerTag);	
	}
	return forReturn;
}


void Actor::SetLayer(int layerIndex)
{
	theWorld.UpdateLayer(this, layerIndex);
}

void Actor::SetLayer(String layerName)
{
	theWorld.UpdateLayer(this, layerName);
}

//Static console command defs

void ActorFactorySetSize( const String& input )
{
	ACTORFACTORY_GETDELEGATE(pDel, ActorFactoryDelegate);

	Vector2 size = StringToVector2(input);

	pDel->SetSize( size.X, size.Y );
}

void ActorFactorySetPosition(const String& input)
{
	ACTORFACTORY_GETDELEGATE(pDel, ActorFactoryDelegate);

	Vector2 pos = StringToVector2(input);

	pDel->SetPosition( pos.X, pos.Y );
}

void ActorFactorySetRotation(const String& input)
{
	ACTORFACTORY_GETDELEGATE(pDel, ActorFactoryDelegate);

	pDel->SetRotation( StringToFloat(input) );
}

void ActorFactorySetColor(const String& input)
{
	ACTORFACTORY_GETDELEGATE(pDel, ActorFactoryDelegate);

	float r,g,b;
	r=g=b=0.0f;

	StringList colors = SplitString(input);
	int size = colors.size();
	if( size > 0 )
		r = StringToFloat(colors[0]);
	if( size > 1 )
		g = StringToFloat(colors[1]);
	if( size > 2 )
		b = StringToFloat(colors[2]);

	pDel->SetColor( r, g, b	);

}

void ActorFactorySetAlpha(const String& input)
{
	ACTORFACTORY_GETDELEGATE(pDel, ActorFactoryDelegate);

	pDel->SetAlpha( StringToFloat(input) );
}

void ActorFactorySetTag(const String& input)
{
	ACTORFACTORY_GETDELEGATE(pDel, ActorFactoryDelegate);

	pDel->SetTag( input );
}

void ActorFactorySetName(const String& input)
{
	ACTORFACTORY_GETDELEGATE(pDel, ActorFactoryDelegate);

	pDel->SetName( input );
}

void ActorFactorySetLayer(const String& input)
{
	ACTORFACTORY_GETDELEGATE(pDel, ActorFactoryDelegate);

	pDel->SetLayer( StringToInt(input) );
}

void ActorFactorySetLayerName(const String& input)
{
	ACTORFACTORY_GETDELEGATE(pDel, ActorFactoryDelegate);

	pDel->SetLayerName( TrimString(input) );
}

void ActorFactorySetSprite(const String& input)
{
	ACTORFACTORY_GETDELEGATE(pDel, ActorFactoryDelegate);

	pDel->SetSprite( input, 0 );
}

void ActorFactoryLoadSpriteFrames(const String& input)
{
	ACTORFACTORY_GETDELEGATE(pDel, ActorFactoryDelegate);

	pDel->LoadSpriteFrames( input );
}

void ActorFactoryAddTag( const String& input )
{
	ACTORFACTORY_GETDELEGATE(pDel, ActorFactoryDelegate);

	pDel->Tag( TrimString(input) );
}

void ActorFactoryDelegate::RegisterOriginalConsoleCommands()
{
	CONSOLE_DECLARECMDSTATICFLAGS( ActorFactorySetSize, ActorFactorySetSize, ConsoleCommand::CCF_CONFIG );
	CONSOLE_DECLARECMDSTATICFLAGS( ActorFactorySetPosition, ActorFactorySetPosition, ConsoleCommand::CCF_CONFIG);
	CONSOLE_DECLARECMDSTATICFLAGS( ActorFactorySetRotation, ActorFactorySetRotation, ConsoleCommand::CCF_CONFIG);
	CONSOLE_DECLARECMDSTATICFLAGS( ActorFactorySetColor, ActorFactorySetColor, ConsoleCommand::CCF_CONFIG );
	CONSOLE_DECLARECMDSTATICFLAGS( ActorFactorySetAlpha, ActorFactorySetAlpha, ConsoleCommand::CCF_CONFIG );
	CONSOLE_DECLARECMDSTATICFLAGS( ActorFactorySetTag, ActorFactorySetTag, ConsoleCommand::CCF_CONFIG );
	CONSOLE_DECLARECMDSTATICFLAGS( ActorFactorySetName, ActorFactorySetName, ConsoleCommand::CCF_CONFIG );
	CONSOLE_DECLARECMDSTATICFLAGS( ActorFactorySetLayer, ActorFactorySetLayer, ConsoleCommand::CCF_CONFIG );
	CONSOLE_DECLARECMDSTATICFLAGS( ActorFactorySetLayerName, ActorFactorySetLayerName, ConsoleCommand::CCF_CONFIG );
	CONSOLE_DECLARECMDSTATICFLAGS( ActorFactorySetSprite, ActorFactorySetSprite, ConsoleCommand::CCF_CONFIG );
	CONSOLE_DECLARECMDSTATICFLAGS( ActorFactoryLoadSpriteFrames, ActorFactoryLoadSpriteFrames, ConsoleCommand::CCF_CONFIG);
	CONSOLE_DECLARECMDSTATICFLAGS( ActorFactoryAddTag, ActorFactoryAddTag, ConsoleCommand::CCF_CONFIG );
}

void ActorFactoryDelegate::InitializeDelegate()
{
	_renderLayer = 1;
}

Actor* ActorFactoryDelegate::CreateInstance()
{
	return new Actor();
}

void ActorFactoryDelegate::SetSize( float x, float y )
{
	ACTORFACTORY_GETACTOR( pActor, Actor );

	pActor->SetSize( x, y );
}

void ActorFactoryDelegate::SetPosition(float x, float y)
{
	ACTORFACTORY_GETACTOR( pActor, Actor );

	pActor->SetPosition( x, y );

}

void ActorFactoryDelegate::SetRotation(float rotation)
{
	ACTORFACTORY_GETACTOR( pActor, Actor );

	pActor->SetRotation( rotation );
}

void ActorFactoryDelegate::SetColor(float r, float g, float b)
{
	ACTORFACTORY_GETACTOR( pActor, Actor );

	pActor->SetColor( r, g, b );
}

void ActorFactoryDelegate::SetAlpha(float newAlpha)
{
	ACTORFACTORY_GETACTOR( pActor, Actor );

	pActor->SetAlpha( newAlpha );

}

void ActorFactoryDelegate::SetTag(String tag)
{
	ACTORFACTORY_GETACTOR( pActor, Actor );

	pActor->Tag( tag );
}

void ActorFactoryDelegate::SetName(String name)
{
	ACTORFACTORY_GETACTOR( pActor, Actor );

	pActor->SetName(name);
}

void ActorFactoryDelegate::SetLayer(int layerIndex)
{
	_renderLayer = layerIndex;
}

void ActorFactoryDelegate::SetLayerName(String layerName )
{
	_renderLayer = theWorld.GetLayerByName(layerName);
}


void ActorFactoryDelegate::SetSprite(String filename, int frame )
{
	ACTORFACTORY_GETACTOR( pActor, Actor );

	pActor->SetSprite( filename, frame );
}

void ActorFactoryDelegate::LoadSpriteFrames(String firstFilename)
{
	ACTORFACTORY_GETACTOR( pActor, Actor );

	pActor->LoadSpriteFrames( firstFilename );

}

void ActorFactoryDelegate::Tag( const String& tag )
{
	ACTORFACTORY_GETACTOR( pActor, Actor );

	if( tag.length() == 0 )
		return;

	pActor->Tag( tag );

}

