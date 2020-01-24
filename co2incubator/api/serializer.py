
from rest_framework import serializers
from .models import estufa1_data

class estufa_data_serializer(serializers.ModelSerializer):

    class Meta:

        model = estufa1_data
        fields = '__all__'