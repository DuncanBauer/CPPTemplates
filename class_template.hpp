#ifndef TEMPLATECLASS_H
#define TEMPLATECLASS_H

class TemplateClass
{
    private:
        /*****************
         * Typedefs
         ****************/

    public:
        /*****************
         * Constructors
         ****************/
        // Default constructor
        TemplateClass() = default;

        // Parameterized constructors
		TemplateClass(int _exampleInt) : exampleInt(_exampleInt) {}
        
        // Copy Constructor
        // Don't deep copy textures / models
        TemplateClass(const TemplateClass& other){}

        // Move Constructor
        TemplateClass(TemplateClass&& other){}

        // Destructor
        ~TemplateClass() { texture = nullptr; }

        /*****************
         * Overloaded Operators
         ****************/

        /*****************
         * Getters and Setters
         ****************/

    private:
        /*****************
         * Variable Members
         ****************/
};

#endif